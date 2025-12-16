#include "lexer.hpp"
#include "token_type.hpp"
#include <cstdio>

bool Lexer::isAtEnd() {
    return current >= source.length();
}

char Lexer::advance() {
    return source.at(current++);
}

void Lexer::addToken(TokenType type) {
    std::string_view text{ source.data() + start, current - start };
    tokens.emplace_back(type, text, line);
}


void Lexer::lexToken() {
    using enum TokenType;
    char c = advance();
    switch (c) {
        case '(': addToken(LEFT_PAREN); break;
        case ')': addToken(RIGHT_PAREN); break;
        case '[': addToken(LEFT_SQ_BRACKET); break;
        case ']': addToken(RIGHT_SQ_BRACKET); break;
        case ',': addToken(COMMA); break;
        case '#': addToken(HASH); break;
        case '=': heading(); break;
    }
}


void Lexer::heading() {
    while (!isAtEnd() && source.at(current) == '=') advance();
    
    addToken(TokenType::HEADING_MARK);
    // can determine heading lvl with token.lexeme_.size();
}


std::vector<Token> Lexer::lexTokens() {
    while (!isAtEnd()) {
        start = current;
        lexToken();

        tokens.push_back(Token(TokenType::EOF_, "", line));
    }
    return tokens;
}