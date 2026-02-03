#include "jit.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>

// Simple x64 Assembler helper
class CodeEmitter {
    std::vector<uint8_t> code;
    // Map from label_id to list of locations that point to it (patch sites)
    std::map<int, std::vector<size_t>> label_patches;
    // Map from label_id to resolved definition location
    std::map<int, size_t> label_defs;
    int next_label_id = 1;

public:
    void* get_code() {
        return code.data();
    }
    
    size_t size() { return code.size(); }

    int alloc_label() { return next_label_id++; }

    void emit_byte(uint8_t b) {
        code.push_back(b);
    }
    
    void emit_bytes(const std::vector<uint8_t>& bytes) {
        code.insert(code.end(), bytes.begin(), bytes.end());
    }
    
    void emit_u32(uint32_t val) {
        code.push_back(val & 0xFF);
        code.push_back((val >> 8) & 0xFF);
        code.push_back((val >> 16) & 0xFF);
        code.push_back((val >> 24) & 0xFF);
    }

    // Call definition of a label (current location)
    void label(int id) {
        label_defs[id] = code.size();
        if (label_patches.count(id)) {
            for (size_t loc : label_patches[id]) {
                // Calculate relative offset
                // rel32 = target - (loc + 4)
                int32_t rel = (int32_t)(code.size() - (loc + 4));
                // Patch
                code[loc] = rel & 0xFF;
                code[loc+1] = (rel >> 8) & 0xFF;
                code[loc+2] = (rel >> 16) & 0xFF;
                code[loc+3] = (rel >> 24) & 0xFF;
            }
            label_patches.erase(id);
        }
    }

    // Emit JMP/Jcc to label (32-bit relative)
    // Opcode is the byte(s) for the jump instruction before the immediate
    void emit_jump(std::vector<uint8_t> opcode, int target_label) {
        emit_bytes(opcode);
        size_t patch_loc = code.size();
        emit_u32(0); // placeholder
        
        if (label_defs.count(target_label)) {
            // Already defined
            int32_t rel = (int32_t)(label_defs[target_label] - (patch_loc + 4));
            code[patch_loc] = rel & 0xFF;
            code[patch_loc+1] = (rel >> 8) & 0xFF;
            code[patch_loc+2] = (rel >> 16) & 0xFF;
            code[patch_loc+3] = (rel >> 24) & 0xFF;
        } else {
            label_patches[target_label].push_back(patch_loc);
        }
    }

    // LEA rax, [rip + label]
    void emit_lea_rip(int target_label) {
        // 48 8D 05 xx xx xx xx
        emit_bytes({0x48, 0x8D, 0x05});
        size_t patch_loc = code.size();
        emit_u32(0);

        if (label_defs.count(target_label)) {
             int32_t rel = (int32_t)(label_defs[target_label] - (patch_loc + 4));
             // patch logic (extracted)
             code[patch_loc] = rel & 0xFF;
             code[patch_loc+1] = (rel >> 8) & 0xFF;
             code[patch_loc+2] = (rel >> 16) & 0xFF;
             code[patch_loc+3] = (rel >> 24) & 0xFF;
        } else {
            label_patches[target_label].push_back(patch_loc);
        }
    }

    // --- Instructions ---

    void push_rdi() { emit_byte(0x57); }
    void pop_rdi() { emit_byte(0x5F); }
    void push_rax() { emit_byte(0x50); }
    void push_rbp() { emit_byte(0x55); }
    void pop_rbp() { emit_byte(0x5D); }
    void ret() { emit_byte(0xC3); }
    
    // mov rsp, rbp
    void mov_rsp_rbp() { emit_bytes({0x48, 0x89, 0xEC}); }
    
    // mov rbp, rsp
    void mov_rbp_rsp() { emit_bytes({0x48, 0x89, 0xE5}); }

    // mov rax, imm64 (simplified to imm32 for 0/1)
    void mov_rax_0() { emit_bytes({0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00}); }
    void mov_rax_1() { emit_bytes({0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00}); }
    // inc rdi
    void inc_rdi() { emit_bytes({0x48, 0xFF, 0xC7}); }

    // mov al, [rdi]
    void mov_al_ptr_rdi() { emit_bytes({0x8A, 0x07}); }

    // cmp al, imm8
    void cmp_al(uint8_t val) { emit_bytes({0x3C, val}); }
    
    // cmp byte ptr [rdi], imm8
    void cmp_ptr_rdi(uint8_t val) { emit_bytes({0x80, 0x3F, val}); }

    // je label
    void je(int label) { emit_jump({0x0F, 0x84}, label); }
    
    // jne label
    void jne(int label) { emit_jump({0x0F, 0x85}, label); }
    
    // jmp label
    void jmp(int label) { emit_jump({0xE9}, label); }
    
    // cmp rdi, rsi (compare pointers)
    void cmp_rdi_rsi() { emit_bytes({0x48, 0x39, 0xF7}); }
};

struct JIT::Impl {
    CodeEmitter emit;
    void* exec_mem = nullptr;
    size_t exec_size = 0;

    ~Impl() {
        if (exec_mem) {
            munmap(exec_mem, exec_size);
        }
    }

    void compile_node(std::shared_ptr<Node> node) {
        switch (node->type) {
            case NODE_CHAR: {
                auto n = std::static_pointer_cast<CharNode>(node);
                // cmp byte ptr [rdi], c
                emit.cmp_ptr_rdi((uint8_t)n->c);
                int fail = emit.alloc_label();
                int success = emit.alloc_label();
                emit.je(success);
                // check for fail
                emit.label(fail);
                emit.pop_rdi();
                emit.ret(); // Backtrack
                
                emit.label(success);
                emit.inc_rdi();
                break;
            }
            case NODE_ANY: {
                // . matches anything except \0? (Usually matches everything except newline, but simple grep matches anything except \0 usually?)
                // Let's implement . does not match \0 (end of string).
                emit.cmp_ptr_rdi(0);
                int fail = emit.alloc_label();
                int success = emit.alloc_label();
                emit.jne(success);
                
                emit.label(fail);
                emit.pop_rdi();
                emit.ret();
                
                emit.label(success);
                emit.inc_rdi();
                break;
            }
            case NODE_CONCAT: {
                auto n = std::static_pointer_cast<ConcatNode>(node);
                compile_node(n->left);
                compile_node(n->right);
                break;
            }
            case NODE_OR: {
                auto n = std::static_pointer_cast<OrNode>(node);
                // push backtrack(B)
                int label_B = emit.alloc_label();
                int label_end = emit.alloc_label();
                
                emit.emit_lea_rip(label_B);
                emit.push_rax();
                emit.push_rdi(); // save valid state
                
                compile_node(n->left);
                
                // If A succeeds, we need to skip B's part?
                // Wait. As analyzed, if A succeeds, we just flow through. 
                // But the backtrack stack still has "Try B". 
                // That is correct. If later A fails, we pop back to B.
                // We do NOT jump to label_end effectively skipping B logic from *execution flow*.
                // Wait. Logic check:
                // Flow: Setup B -> Run A -> continue.
                // If A fails (internally): it pops [rdi, label_B] and jumps to label_B.
                // So at label_B we are in the failure path of A.
                
                // But wait, if A succeeds, we continue execution.
                // We should NOT encounter `label_B` during normal flow.
                // So we need to JUMP over it.
                // But `label_B` is a target address, not a block of code? 
                // Ah, `label_B` is where the CODE for B starts.
                
                // Oops.
                // [Setup Backtrack B]
                // [Code for A] -- if success, falls through.
                // JMP End -- If A succeeds, we skip trying B *now*.
                
                // label_B:
                //   pop rdi
                //   [Code for B]
                
                // End:
                //   continue
                
                // Re-verification:
                // If A succeeds, we jump to End. Stack has [B].
                // If subsequent nodes fail, they pop [B] and jump to label_B. Correct.
                
                emit.jmp(label_end);
                
                emit.label(label_B);
                // emit.pop_rdi(); // REMOVED: Stack is clean, RDI is restored by fail handler
                compile_node(n->right);
                
                emit.label(label_end);
                break;
            }
            case NODE_STAR: {
                auto n = std::static_pointer_cast<StarNode>(node);
                // Greedy loop
                int loop_start = emit.alloc_label();
                int next_alt = emit.alloc_label();
                
                emit.label(loop_start);
                // Push backtrack to next_alt (Done)
                emit.emit_lea_rip(next_alt);
                emit.push_rax();
                emit.push_rdi();
                
                compile_node(n->child);
                // If child succeeds, loop back
                emit.jmp(loop_start);
                
                emit.label(next_alt); // This is the backtrack target (Exit loop)
                // emit.pop_rdi(); // REMOVED: Stack is clean, RDI is restored
                // Fall through to rest
                break;
            }
            case NODE_START: {
                // check rdi == rsi
                emit.cmp_rdi_rsi();
                int fail = emit.alloc_label();
                int success = emit.alloc_label();
                emit.je(success);
                
                emit.label(fail);
                emit.pop_rdi();
                emit.ret();
                
                emit.label(success);
                break;
            }
            case NODE_END: {
                // check [rdi] == 0 or \n (depending on line)
                // Assuming null terminated string as input for line
                emit.cmp_ptr_rdi(0);
                int fail = emit.alloc_label();
                int success = emit.alloc_label();
                emit.je(success);
                
                emit.label(fail);
                emit.pop_rdi();
                emit.ret();
                
                emit.label(success);
                break;
            }
        }
    }
    
    void finalize() {
        // Allocate executable memory
        exec_size = emit.size();
        FILE* f = fopen("jit_dump.bin", "wb");
        if(f) { fwrite(emit.get_code(), 1, exec_size, f); fclose(f); }

        exec_mem = mmap(nullptr, exec_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);        if (exec_mem == MAP_FAILED) {
            perror("mmap");
            return;
        }
        memcpy(exec_mem, emit.get_code(), exec_size);
        mprotect(exec_mem, exec_size, PROT_READ | PROT_EXEC);
    }
};

JIT::JIT() : impl(std::make_unique<Impl>()) {}
JIT::~JIT() = default;

void JIT::compile(std::shared_ptr<Node> root) {
    // Prologue
    impl->emit.push_rbp();
    impl->emit.mov_rbp_rsp();
    
    // Global Fail Handler
    int global_fail = impl->emit.alloc_label();
    impl->emit.emit_lea_rip(global_fail);
    impl->emit.push_rax();
    impl->emit.push_rdi(); // dummy rdi
    
    // Compile AST
    if (root) {
        impl->compile_node(root);
    }
    
    // Success (Machine code fell through all nodes)
    // Return 1
    impl->emit.mov_rax_1();
    impl->emit.mov_rsp_rbp(); // Restore stack (clears backtrack stack)
    impl->emit.pop_rbp();
    impl->emit.ret();
    
    // Global Fail Target
    impl->emit.label(global_fail);
    impl->emit.pop_rdi(); // consume dummy rdi
    impl->emit.mov_rax_0();
    impl->emit.mov_rsp_rbp();
    impl->emit.pop_rbp();
    impl->emit.ret();
    
    impl->finalize();
}

typedef bool (*match_func_t)(const char* text, const char* start);

bool JIT::execute(const char* text, const char* text_start) {
    if (!impl->exec_mem) return false;
    auto func = (match_func_t)impl->exec_mem;
    return func(text, text_start);
}
