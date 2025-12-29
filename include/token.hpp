#pragma once

#include <string_view>
#include <variant>
#include "source.hpp"
#include "token_type.hpp"

using Literal = std::variant<std::monostate, double, std::string_view>;
class Token {
public:
    Token(TokenType type, std::string_view lexeme, SourceSpan span, Literal value);
    TokenType getType() const { return type_; }
    std::string_view getLexeme() const { return lexeme_; }
    const SourceSpan& span() const noexcept { return span_; }
private:
    TokenType type_;
    std::string_view lexeme_;
    SourceSpan span_;
    Literal value_;
};
