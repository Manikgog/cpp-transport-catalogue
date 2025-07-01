#include "json_builder.h"

namespace json {

Builder& Builder::Value(Node value) {
    CheckNotReady();
    CheckArrayOrDict();

    Node* current = GetCurrentNode();
    if (current == nullptr) {
        root_ = std::move(value);
    } else if (current->IsArray()) {
        const_cast<Array&>(current->AsArray()).push_back(std::move(value));
    } else if (has_key_) {
        const_cast<Dict&>(current->AsDict()).emplace(std::move(last_key_), std::move(value));
        has_key_ = false;
    } else {
        throw std::logic_error("Value called without Key in Dict");
    }

    return *this;
}

DictItemContext Builder::StartDict() {
    CheckNotReady();
    CheckArrayOrDict();

    Node* current = GetCurrentNode();
    if (current == nullptr) {
        root_ = Dict{};
        nodes_stack_.push_back(&root_);
    } else if (current->IsArray()) {
        const_cast<Array&>(current->AsArray()).emplace_back(Dict{});
        nodes_stack_.push_back(&const_cast<Array&>(current->AsArray()).back());
    } else if (has_key_) {
        const_cast<Dict&>(current->AsDict()).emplace(last_key_, Dict{});
        nodes_stack_.push_back(&const_cast<Dict&>(current->AsDict()).at(last_key_));
        has_key_ = false;
    } else {
        throw std::logic_error("StartDict called without Key in Dict");
    }

    return DictItemContext(*this);
}

    ArrayItemContext Builder::StartArray() {
    CheckNotReady();
    CheckArrayOrDict();

    Node* current = GetCurrentNode();
    if (current == nullptr) {
        root_ = Array{};
        nodes_stack_.push_back(&root_);
    } else if (current->IsArray()) {
        const_cast<Array&>(current->AsArray()).emplace_back(Array{});
        nodes_stack_.push_back(&const_cast<Array&>(current->AsArray()).back());
    } else if (has_key_) {
        const_cast<Dict&>(current->AsDict()).emplace(last_key_, Array{});
        nodes_stack_.push_back(&const_cast<Dict&>(current->AsDict()).at(last_key_));
        has_key_ = false;
    } else {
        throw std::logic_error("StartArray called without Key in Dict");
    }

    return ArrayItemContext(*this);
}

Builder& Builder::Key(std::string key) {
    CheckNotReady();
    CheckInDict();
    if (has_key_) {
        throw std::logic_error("Key called twice in a row");
    }

    last_key_ = std::move(key);
    has_key_ = true;
    return *this;
}

Builder& Builder::EndDict() {
    CheckNotReady();
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("EndDict called without StartDict");
    }

    nodes_stack_.pop_back();
    return *this;
}

Builder& Builder::EndArray() {
    CheckNotReady();
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("EndArray called without StartArray");
    }

    nodes_stack_.pop_back();
    return *this;
}

Node Builder::Build() {
    CheckReady();
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Build called with unfinished container");
    }

    return std::move(root_);
}

    void Builder::CheckReady() const {
    if (root_.IsNull() && nodes_stack_.empty()) {
        throw std::logic_error("Build called on empty Builder");
    }
}

void Builder::CheckNotReady() const {
    if (!root_.IsNull() && nodes_stack_.empty()) {
        throw std::logic_error("Method called after Build");
    }
}

void Builder::CheckInDict() const {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Key called outside of Dict");
    }
}

void Builder::CheckNotInDict() const {
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && !has_key_) {
        throw std::logic_error("Value called in Dict without Key");
    }
}

void Builder::CheckArrayOrDict() const {
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && !has_key_) {
        throw std::logic_error("Value called in Dict without Key");
    }
}

Node* Builder::GetCurrentNode() {
    return nodes_stack_.empty() ? nullptr : nodes_stack_.back();
}

    DictItemContext::DictItemContext(Builder& builder) : builder_(builder) {}

    KeyItemContext DictItemContext::Key(std::string key) const {
    builder_.Key(std::move(key));
    return KeyItemContext(builder_);
}

    Builder& DictItemContext::EndDict() const {
    return builder_.EndDict();
}

    KeyItemContext::KeyItemContext(Builder& builder) : builder_(builder) {}

    DictItemContext KeyItemContext::Value(Node value) const {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

    DictItemContext KeyItemContext::StartDict() const {
    return builder_.StartDict();
}

    ArrayItemContext KeyItemContext::StartArray() const {
    return builder_.StartArray();
}

    ArrayItemContext::ArrayItemContext(Builder& builder) : builder_(builder) {}

    ArrayItemContext& ArrayItemContext::Value(Node value) {
    builder_.Value(std::move(value));
    return *this;
}

    DictItemContext ArrayItemContext::StartDict() const {
    return builder_.StartDict();
}

    ArrayItemContext ArrayItemContext::StartArray() const {
    return builder_.StartArray();
}

    Builder& ArrayItemContext::EndArray() const {
    return builder_.EndArray();
}

} // namespace json