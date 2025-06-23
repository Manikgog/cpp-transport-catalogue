#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

        Node() = default;
        Node(std::nullptr_t);
        Node(Array array);
        Node(Dict map);
        Node(int value);
        Node(double value);
        Node(bool value);
        Node(std::string value);

        bool IsNull() const;
        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        bool AsBool() const;
        double AsDouble() const;
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        const Value& GetValue() const { return value_; }

        bool operator==(const Node& rhs) const;
        bool operator!=(const Node& rhs) const;

    private:
        Value value_;
    };

    class Document {
    public:
        explicit Document(Node root);
        const Node& GetRoot() const;

        bool operator==(const Document& rhs) const;
        bool operator!=(const Document& rhs) const;

    private:
        Node root_;
    };

    struct PrintContext {
        std::ostream& out_;
        int indent_step_ = 4;
        int indent_ = 4;

        PrintContext(std::ostream& out);

        PrintContext(std::ostream& out, int indent_step, int indent);

        void PrintIndent() const;

        PrintContext Indented() const;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

}  // namespace json