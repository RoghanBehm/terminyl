#pragma once

#include <string>
#include <vector>
#include "token.hpp"
#include "token_type.hpp"
#include "error.hpp"

class Lexer {
public:
    explicit Lexer(const std::string& source);

    Token next();
    char peek();
    bool match(char expected);
    std::vector<Token> scan_tokens();
    void addToken(TokenType type);
    void addToken(TokenType type, Literal lit);
    void lexToken();
    void heading();
    std::vector<Token> lexTokens();
    const std::string& getSource() const { return source_; }
    const DiagnosticSet& diagnostics() const { return diagnostics_; }

private:
    char peekNext();
    char advance();
    void skip_spaces();
    void text();
    Token ident_or_text();
    Token punctuation();
    void number();
    void string();
    bool isAtEnd();
    bool isDigit(char c);
    bool isSpecialChar(char c);
    const std::string& source_;
    std::size_t start = 0;
    std::size_t current = 0;
    SourcePos start_pos{1, 1};
    SourcePos cur_pos{1, 1};
    std::vector<Token> tokens;
    DiagnosticSet diagnostics_;
    void error(std::string message, SourceSpan span);
    
};