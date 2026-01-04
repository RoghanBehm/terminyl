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
    void addToken(TokenType type, Literal value);
    void lexToken();
    void heading();
    std::vector<Token> lexTokens();
    const std::string& getSource() const { return source_; }
    const DiagnosticSet& diagnostics() const { return diagnostics_; }

private:
    enum class LexerMode : std::uint8_t {
        TEXT,       // Normal text mode - spaces significant
        EXPRESSION  // Expression mode - skip whitespace
    };
    
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
    LexerMode mode_ = LexerMode::TEXT;
    int paren_depth_ = 0;
    void error(std::string message, SourceSpan span);
    
};