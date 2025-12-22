#include "parser.hpp"

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
    Document::Paragraph paragraph;
    paragraph.span.start = peek().span().start;

    std::string text;
    if (check(TokenType::TEXT)) {
        const Token& t = advance();
        text = std::string(t.getLexeme());
    }

    if (check(TokenType::NEWLINE)) advance();

    paragraph.text = std::move(text);
    paragraph.span.end = previous().span().end;
    return paragraph;
}

Document::Block Parser::block() {
    if (check(TokenType::HEADING_MARK)) return heading();
    return paragraph();
}

const Token& Parser::peek() const { return tokens_.at(current); }

const Token& Parser::previous() const { return tokens_.at(current - 1); }


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