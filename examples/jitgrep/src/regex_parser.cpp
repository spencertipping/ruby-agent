#include "regex.h"
#include <stdexcept>
#include <string>
#include <vector>

class RegexParser {
public:
    explicit RegexParser(const std::string& pattern)
        : pattern_(pattern), pos_(0) {}

    std::shared_ptr<Node> parse() {
        if (pattern_.empty()) return nullptr;
        auto node = parseOr();
        if (pos_ < pattern_.length()) {
            throw std::runtime_error("Unexpected character at end of regex");
        }
        return node;
    }

private:
    std::string pattern_;
    size_t pos_;

    char peek() const {
        if (pos_ < pattern_.length()) {
            return pattern_[pos_];
        }
        return '\0';
    }

    char advance() {
        if (pos_ < pattern_.length()) {
            return pattern_[pos_++];
        }
        return '\0';
    }

    // Lowest precedence: |
    std::shared_ptr<Node> parseOr() {
        auto node = parseConcat();
        while (peek() == '|') {
            advance(); // consume '|'
            auto right = parseConcat();
            node = std::make_shared<OrNode>(node, right);
        }
        return node;
    }

    // Mid precedence: Concatenation (implicit)
    std::shared_ptr<Node> parseConcat() {
        std::shared_ptr<Node> node = nullptr;

        while (pos_ < pattern_.length() && peek() != '|' && peek() != ')') {
            auto next = parseStar();
            if (!node) {
                node = next;
            } else {
                node = std::make_shared<ConcatNode>(node, next);
            }
        }

        return node;
    }

    // High precedence: *
    std::shared_ptr<Node> parseStar() {
        auto node = parsePrimary();
        while (peek() == '*') {
            advance(); // consume '*'
            if (node == nullptr) {
                throw std::runtime_error("Nothing to repeat before *");
            }
            node = std::make_shared<StarNode>(node);
        }
        return node;
    }

    // Highest precedence: atoms, (), ^, $, .
    std::shared_ptr<Node> parsePrimary() {
        char c = peek();
        if (c == '(') {
            advance(); // consume '('
            auto node = parseOr();
            if (advance() != ')') {
                throw std::runtime_error("Unbalanced parentheses");
            }
            return node;
        } else if (c == '.') {
            advance();
            return std::make_shared<AnyNode>();
        } else if (c == '^') {
            advance();
            return std::make_shared<StartNode>();
        } else if (c == '$') {
            advance();
            return std::make_shared<EndNode>();
        } else if (c == '\\') {
            advance(); // consume '\'
            char escaped = advance();
            if (escaped == '\0') throw std::runtime_error("Trailing backslash");
            return std::make_shared<CharNode>(escaped);
        } else if (c == '*' || c == '|' || c == ')') {
            // These characters are syntactically significant and handled by callers.
            // If they appear here unexpectedly, it may be an empty branch of Or or Concat.
            return nullptr;
        } else if (c == '\0') {
            return nullptr;
        } else {
            advance();
            return std::make_shared<CharNode>(c);
        }
    }
};

std::shared_ptr<Node> parse_regex(const std::string& pattern) {
    RegexParser parser(pattern);
    return parser.parse();
}
