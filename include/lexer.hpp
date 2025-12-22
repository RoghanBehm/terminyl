#pragma once

#include <string>
#include <vector>
#include "token.hpp"
#include "token_type.hpp"

class Lexer {
public:
    explicit Lexer(std::string source);

    Token next();
    char peek();
    std::vector<Token> scan_tokens();
    void addToken(TokenType type);
    void lexToken();
    void heading();
    std::vector<Token> lexTokens();
private:
    char advance();
    void skip_spaces();
    void text();
    Token ident_or_text();
    Token punctuation();
    bool isAtEnd();
    const std::string source;
    std::size_t start = 0;
    std::size_t current = 0;
    SourcePos start_pos{1, 1};
    SourcePos cur_pos{1, 1};
    std::size_t line = 1;
    std::vector<Token> tokens;
    
};