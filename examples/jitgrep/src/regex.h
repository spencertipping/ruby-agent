#pragma once
#include <memory>
#include <string>
#include <vector>

enum NodeType {
    NODE_CHAR,
    NODE_ANY, // .
    NODE_CONCAT,
    NODE_STAR,
    NODE_OR,
    NODE_START, // ^
    NODE_END    // $
};

struct Node {
    NodeType type;
    virtual ~Node() = default;
};

struct CharNode : public Node {
    char c;
    CharNode(char c) : c(c) { type = NODE_CHAR; }
};

struct AnyNode : public Node {
    AnyNode() { type = NODE_ANY; }
};

struct ConcatNode : public Node {
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    ConcatNode(std::shared_ptr<Node> l, std::shared_ptr<Node> r) : left(l), right(r) { type = NODE_CONCAT; }
};

struct StarNode : public Node {
    std::shared_ptr<Node> child;
    StarNode(std::shared_ptr<Node> c) : child(c) { type = NODE_STAR; }
};

struct OrNode : public Node {
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    OrNode(std::shared_ptr<Node> l, std::shared_ptr<Node> r) : left(l), right(r) { type = NODE_OR; }
};

struct StartNode : public Node {
    StartNode() { type = NODE_START; }
};

struct EndNode : public Node {
    EndNode() { type = NODE_END; }
};

std::shared_ptr<Node> parse_regex(const std::string& pattern);
