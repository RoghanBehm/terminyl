#include "lexer.hpp"
#include "token_type.hpp"
#include <cstdio>

Lexer::Lexer(std::string& source) : source_(source) {}

bool Lexer::isAtEnd() {
    return current >= getSource().length();
}

char Lexer::advance() {
    char c = getSource().at(current++);
    if (c == '\n') {
        cur_pos.line++;
        cur_pos.column = 1;
    } else {
        cur_pos.column++;
    }
    return c;
}


void Lexer::addToken(TokenType type) {
    std::string_view text{ getSource().data() + start, current - start };
    SourceSpan span{ start_pos, cur_pos };
    tokens.emplace_back(type, text, span);
}


char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return getSource().at(current);
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
        case '\n': addToken(NEWLINE); break;
        case '=': heading(); break;
        default: text(); break;
    }
}


void Lexer::heading() {
    while (!isAtEnd() && getSource().at(current) == '=') advance();
    
    addToken(TokenType::HEADING_MARK);
    // can determine heading lvl with token.lexeme_.size();
}


std::vector<Token> Lexer::lexTokens() {
    while (!isAtEnd()) {
        start = current;
        start_pos = cur_pos;
        lexToken();

        
    }
    tokens.emplace_back(TokenType::EOF_, std::string_view{}, SourceSpan{cur_pos, cur_pos});
    return tokens;
}

void Lexer::text() {
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }

    addToken(TokenType::TEXT);
}