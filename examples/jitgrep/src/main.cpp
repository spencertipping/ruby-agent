#include <iostream>
#include <string>
#include <vector>
#include "regex.h"
#include "jit.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <pattern>" << std::endl;
        return 1;
    }

    std::string pattern = argv[1];

    try {
        auto root = parse_regex(pattern);
        if (!root) {
            std::cerr << "Empty regex parsed." << std::endl;
            return 1;
        }

        JIT jit;
        jit.compile(root);

        std::string line;
        while (std::getline(std::cin, line)) {
            bool matched = false;
            const char* start = line.c_str();
            
            // Try matching at every position
            for (size_t i = 0; i <= line.length(); ++i) {
                if (jit.execute(start + i, start)) {
                    matched = true;
                    break;
                }
            }

            if (matched) {
                std::cout << line << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
