#pragma once

#include <string>
#include "source.hpp"
#include "token_type.hpp"

class Token {
public:
    Token(TokenType type, std::string_view lexeme, std::size_t line);

private:
    TokenType type_;
    std::string_view lexeme_;
    std::size_t line_;
};
