#pragma once

#include "json.h"
#include <string>
#include <vector>

namespace json {

    class Builder {
    public:
        Builder() = default;

        class DictItemContext StartDict();
        class ArrayItemContext StartArray();

        Node Build();

        Builder& Value(Node value);
        Builder& Key(std::string key);
        Builder& EndDict();
        Builder& EndArray();

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
        std::string last_key_;
        bool has_key_ = false;

        void CheckReady() const;
        void CheckNotReady() const;
        void CheckInDict() const;
        void CheckNotInDict() const;
        void CheckArrayOrDict() const;
        Node* GetCurrentNode();
    };

    class KeyItemContext;

    class DictItemContext {
    public:
        explicit DictItemContext(Builder& builder);
        KeyItemContext Key(std::string key) const;
        Builder& EndDict() const;

    private:
        Builder& builder_;
    };

    class KeyItemContext {
    public:
        explicit KeyItemContext(Builder& builder);
        DictItemContext Value(Node value) const;
        DictItemContext StartDict() const;
        ArrayItemContext StartArray() const;

    private:
        Builder& builder_;
    };

    class ArrayItemContext {
    public:
        explicit ArrayItemContext(Builder& builder);
        ArrayItemContext& Value(Node value);
        DictItemContext StartDict() const;
        ArrayItemContext StartArray() const;
        Builder& EndArray() const;

    private:
        Builder& builder_;
    };

} // namespace json