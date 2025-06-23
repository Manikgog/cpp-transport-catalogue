#include "json.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

using namespace std;

namespace json {
    // Конструкторы Node
    Node::Node(nullptr_t) : value_(nullptr) {}
    Node::Node(Array array) : value_(std::move(array)) {}
    Node::Node(Dict map) : value_(std::move(map)) {}
    Node::Node(int value) : value_(value) {}
    Node::Node(double value) : value_(value) {}
    Node::Node(bool value) : value_(value) {}
    Node::Node(string value) : value_(std::move(value)) {}

    // Методы проверки типа
    bool Node::IsNull() const { return holds_alternative<nullptr_t>(value_); }
    bool Node::IsInt() const { return holds_alternative<int>(value_); }
    bool Node::IsDouble() const { return holds_alternative<double>(value_) || IsInt(); }
    bool Node::IsPureDouble() const { return holds_alternative<double>(value_); }
    bool Node::IsBool() const { return holds_alternative<bool>(value_); }
    bool Node::IsString() const { return holds_alternative<string>(value_); }
    bool Node::IsArray() const { return holds_alternative<Array>(value_); }
    bool Node::IsMap() const { return holds_alternative<Dict>(value_); }

    // Методы получения значения
    int Node::AsInt() const {
        if (!IsInt()) throw logic_error("Not an int");
        return get<int>(value_);
    }

    bool Node::AsBool() const {
        if (!IsBool()) throw logic_error("Not a bool");
        return get<bool>(value_);
    }

    double Node::AsDouble() const {
        if (IsInt()) return get<int>(value_);
        if (!IsPureDouble()) throw logic_error("Not a double");
        return get<double>(value_);
    }

    const string& Node::AsString() const {
        if (!IsString()) throw logic_error("Not a string");
        return get<string>(value_);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) throw logic_error("Not an array");
        return get<Array>(value_);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) throw logic_error("Not a map");
        return get<Dict>(value_);
    }

    // Операторы сравнения
    bool Node::operator==(const Node& rhs) const { return value_ == rhs.value_; }
    bool Node::operator!=(const Node& rhs) const { return !(*this == rhs); }

    // Реализация Document
    Document::Document(Node root) : root_(std::move(root)) {}
    const Node& Document::GetRoot() const { return root_; }

    bool Document::operator==(const Document& rhs) const {
        return root_ == rhs.root_;
    }

    bool Document::operator!=(const Document& rhs) const {
        return !(*this == rhs);
    }

    namespace {

        Node LoadNode(istream& input);

        // Загрузка строки с обработкой escape-последовательностей
        string LoadString(istream& input) {
            string line;
            char c;
            while (input.get(c) && c != '"') {
                if (c == '\\') {
                    // Обработка escape-последовательностей
                    if (!input.get(c)) {
                        throw ParsingError("String parsing error");
                    }
                    switch (c) {
                        case 'n': line += '\n'; break;
                        case 'r': line += '\r'; break;
                        case 't': line += '\t'; break;
                        case '"': line += '"'; break;
                        case '\\': line += '\\'; break;
                        default: throw ParsingError("Invalid escape sequence");
                    }
                } else {
                    line += c;
                }
            }
            if (c != '"') {
                throw ParsingError("String parsing error");
            }
            return line;
        }

        // Загрузка числа (int или double)
        Node LoadNumber(istream& input) {
            string parsed_num;
            bool is_double = false;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream");
                }
            };

            // Считываем возможный минус
            if (input.peek() == '-') {
                read_char();
            }

            // Считываем цифры перед точкой
            if (!isdigit(input.peek())) {
                throw ParsingError("A digit is expected");
            }
            while (isdigit(input.peek())) {
                read_char();
            }

            // Проверяем наличие дробной части
            if (input.peek() == '.') {
                read_char();
                is_double = true;
                if (!isdigit(input.peek())) {
                    throw ParsingError("A digit is expected after decimal point");
                }
                while (isdigit(input.peek())) {
                    read_char();
                }
            }

            // Проверяем экспоненциальную часть
            if (input.peek() == 'e' || input.peek() == 'E') {
                read_char();
                is_double = true;
                if (input.peek() == '+' || input.peek() == '-') {
                    read_char();
                }
                if (!isdigit(input.peek())) {
                    throw ParsingError("A digit is expected in exponent");
                }
                while (isdigit(input.peek())) {
                    read_char();
                }
            }

            try {
                if (is_double) {
                    return stod(parsed_num);
                } else {
                    return stoi(parsed_num);
                }
            } catch (...) {
                throw ParsingError("Failed to convert number");
            }
        }

        // Загрузка массива
        Node LoadArray(istream& input) {
            Array result;

            char c;
            input >> c;
            if (c == ']') {
                return Node(std::move(result)); // Пустой массив
            }
            input.putback(c);

            while (true) {
                result.push_back(LoadNode(input));
                input >> c;
                if (c == ']') {
                    break;
                }
                if (c != ',') {
                    throw ParsingError("Expected ',' or ']' in array");
                }
            }

            return Node(std::move(result));
        }

        // Загрузка словаря
        Node LoadDict(istream& input) {
            Dict result;

            char c;
            input >> c;
            if (c == '}') {
                return Node(std::move(result)); // Пустой словарь
            }
            input.putback(c);

            while (true) {
                input >> c;
                if (c != '"') {
                    throw ParsingError("Expected '\"' as dictionary key");
                }
                string key = LoadString(input);

                input >> c;
                if (c != ':') {
                    throw ParsingError("Expected ':' after dictionary key");
                }

                result.insert({std::move(key), LoadNode(input)});

                input >> c;
                if (c == '}') {
                    break;
                }
                if (c != ',') {
                    throw ParsingError("Expected ',' or '}' in dictionary");
                }
            }

            return Node(std::move(result));
        }

        // Загрузка булевого значения
        Node LoadBool(istream& input) {
            string value;
            char c = input.peek();
            if (c == 't') {
                // Проверяем "true"
                for (const char* expected = "true"; *expected; ++expected) {
                    if (!input) {
                        throw ParsingError("Failed to read bool value");
                    }
                    if (input.get() != *expected) {
                        throw ParsingError("Invalid bool value");
                    }
                }
                // Проверяем, что после "true" идет корректный разделитель
                c = input.peek();
                if (isalnum(c) || c == '_') {
                    throw ParsingError("Invalid bool value");
                }
                return Node(true);
            } else if (c == 'f') {
                // Проверяем "false"
                for (const char* expected = "false"; *expected; ++expected) {
                    if (!input) {
                        throw ParsingError("Failed to read bool value");
                    }
                    if (input.get() != *expected) {
                        throw ParsingError("Invalid bool value");
                    }
                }
                // Проверяем, что после "false" идет корректный разделитель
                c = input.peek();
                if (isalnum(c) || c == '_') {
                    throw ParsingError("Invalid bool value");
                }
                return Node(false);
            } else {
                throw ParsingError("Invalid bool value");
            }
        }

        // Загрузка null
        Node LoadNull(istream& input) {
            // Проверяем "null"
            for (const char* expected = "null"; *expected; ++expected) {
                if (!input) {
                    throw ParsingError("Failed to read null value");
                }
                if (input.get() != *expected) {
                    throw ParsingError("Invalid null value");
                }
            }
            // Проверяем, что после "null" идет корректный разделитель
            char c = input.peek();
            if (isalnum(c) || c == '_') {
                throw ParsingError("Invalid null value");
            }
            return Node(nullptr);
        }

        // Основная функция загрузки узла
        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (!input) {
                throw ParsingError("Unexpected end of input");
            }

            if (c == '[') {
                return LoadArray(input);
            } else if (c == '{') {
                return LoadDict(input);
            } else if (c == '"') {
                return Node(LoadString(input));
            } else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            } else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            } else if (isdigit(c) || c == '-') {
                input.putback(c);
                return LoadNumber(input);
            } else {
                throw ParsingError("Unexpected character: " + string(1, c));
            }
        }

    }  // namespace

    // Загрузка документа
    Document Load(istream& input) {
        return Document{LoadNode(input)};
    }

// Вспомогательные функции для вывода
namespace {

    void PrintValue(nullptr_t, PrintContext ctx) {
        ctx.out_ << "null";
    }

    void PrintValue(bool value, PrintContext ctx) {
        ctx.out_ << (value ? "true" : "false");
    }

    void PrintValue(int value, PrintContext ctx) {
        ctx.out_ << value;
    }

    void PrintValue(double value, PrintContext ctx) {
        ctx.out_ << value;
    }

    void PrintValue(const string& value, PrintContext ctx) {
        ctx.out_ << '"';
        for (char c : value) {
            switch (c) {
                case '\n': ctx.out_ << "\\n"; break;
                case '\r': ctx.out_ << "\\r"; break;
                case '\t': ctx.out_ << "\\t"; break;
                case '"': ctx.out_ << "\\\""; break;
                case '\\': ctx.out_ << "\\\\"; break;
                default: ctx.out_ << c;
            }
        }
        ctx.out_ << '"';
    }

    void PrintValue(const Dict& dict, PrintContext ctx);

    void PrintValue(const Array& array, PrintContext ctx) {
        if (array.empty()) {
            ctx.out_ << "[]";
            return;
        }

        ctx.out_ << "[\n";
        PrintContext inner_ctx = ctx.Indented();
        bool first = true;
        for (const auto& item : array) {
            if (!first) {
                ctx.out_ << ",\n";
            }
            first = false;
            inner_ctx.PrintIndent();
            visit([&inner_ctx](const auto& value) { PrintValue(value, inner_ctx); }, item.GetValue());
        }
        ctx.out_ << "\n";
        ctx.PrintIndent();
        ctx.out_ << "]";
    }

    void PrintValue(const Dict& dict, PrintContext ctx) {
        if (dict.empty()) {
            ctx.out_ << "{}";
            return;
        }

        ctx.out_ << "{\n";
        PrintContext inner_ctx = ctx.Indented();
        bool first = true;
        for (const auto& [key, value] : dict) {
            if (!first) {
                ctx.out_ << ",\n";
            }
            first = false;
            inner_ctx.PrintIndent();
            PrintValue(key, inner_ctx);
            ctx.out_ << ": ";
            visit([&inner_ctx](const auto& val) { PrintValue(val, inner_ctx); }, value.GetValue());
        }
        ctx.out_ << "\n";
        ctx.PrintIndent();
        ctx.out_ << "}";
    }

    }  // namespace

    PrintContext::PrintContext(std::ostream& out)
        : out_(out), indent_step_(4), indent_(0) {}

    PrintContext::PrintContext(std::ostream& out, int indent_step, int indent)
        : out_(out), indent_step_(indent_step), indent_(indent) {}

    void PrintContext::PrintIndent() const {
        for (int i = 0; i < indent_; ++i) {
            out_.put(' ');
        }
    }

    PrintContext PrintContext::Indented() const {
        return {out_, indent_step_, indent_ + indent_step_};
    }

    void Print(const Document& doc, ostream& output) {
        PrintContext ctx{output};
        visit([&ctx](const auto& value) { PrintValue(value, ctx); }, doc.GetRoot().GetValue());
        output << "\n";
    }
}