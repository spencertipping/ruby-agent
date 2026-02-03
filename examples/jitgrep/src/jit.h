#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include "regex.h"

class JIT {
public:
    JIT();
    ~JIT();

    // Compile the AST into machine code
    void compile(std::shared_ptr<Node> root);

    // Run the compiled code against input
    // Returns true if match found at current position
    bool execute(const char* text, const char* text_start);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
