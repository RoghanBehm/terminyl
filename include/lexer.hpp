#pragma once

#include <string>
#include "token.hpp"

class Lexer {
public:
    explicit Lexer(std::string source);

    Token next();
    Token peek() const;
private:
    char current() const;
    char advance();
    void skip_spaces();
    Token lex_string();
    Token lex_ident_or_text();
    Token lex_punctuation();
};