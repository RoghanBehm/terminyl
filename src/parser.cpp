#include "parser.hpp"
#include <cassert>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Document Parser::parse() {
    Document doc;

    
    while (!isAtEnd()) {
        skipBlanks();
        if (isAtEnd()) break;
        doc.add(block());

    }
    return doc;
}

Document::Heading Parser::heading() {
    const Token& token = advance();

    Document::Heading heading;
    heading.level = static_cast<int>(token.getLexeme().size());
    heading.span.start = token.span().start;
    
    std::string text;
    if (check(TokenType::TEXT)) {
        const Token& t = advance();
        text = std::string(t.getLexeme());
    }

    if (check(TokenType::NEWLINE)) advance();

    heading.text = std::move(text);
    heading.span.end = previous().span().end;
    return heading;
}

Document::Paragraph Parser::paragraph() {
    Document::Paragraph p;
    p.span.start = peek().span().start;

    std::string text;
    bool consumed_any = false;

    while (!isAtEnd() && !check(TokenType::NEWLINE)) {
        const Token& t = advance();
        text += std::string(t.getLexeme()); // string_view -> owned
        consumed_any = true;
    }

    if (check(TokenType::NEWLINE)) advance();

    p.text = std::move(text);

    // safe end span
    if (consumed_any) {
        p.span.end = previous().span().end;
    } else {
        // shouldn't happen if caller skipped blanks, but safe anyway
        p.span.end = peek().span().start;
    }

    return p;
}


Document::Block Parser::block() {
    if (check(TokenType::HEADING_MARK)) return heading();
    return paragraph();
}

const Token& Parser::peek() const { return tokens_.at(current); }

const Token& Parser::previous() const {
    assert(current > 0);
    return tokens_.at(current - 1); 
}


const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

void Parser::skipBlanks() {
    while (match(TokenType::NEWLINE)) {
        // keep eating newlines
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    
    return false;
}


bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().getType() == type;
}

bool Parser::isAtEnd() {
    return peek().getType() == TokenType::EOF_;
}