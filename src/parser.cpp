#include "parser.hpp"
#include "token_type.hpp"
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
  const Token &token = advance();

  Document::Heading heading;
  heading.level = static_cast<int>(token.getLexeme().size());
  heading.span.start = token.span().start;

  std::string text;
  if (check(TokenType::TEXT)) {
    const Token &t = advance();
    text = std::string(t.getLexeme());
  }

  if (check(TokenType::NEWLINE))
    advance();

  heading.text = std::move(text);
  heading.span.end = previous().span().end;
  return heading;
}


Document::Paragraph Parser::paragraph() {
    Document::Paragraph para;
    para.span.start = peek().span().start;
    
    para.inlines = parseInlines(TokenType::NEWLINE);
    
    // Skip trailing newlines
    while (match(TokenType::NEWLINE)) {}
    
    para.span.end = previous().span().end;
    return para;
}

// Returns true if paragraph should end
bool Parser::handleNewlineInParagraph(TextAccumulator& text, bool& consumed_any) {
    if (!check(TokenType::NEWLINE)) {
        return false;
    }
    
    // Double newline ends paragraph
    if (current + 1 < tokens_.size() &&
        tokens_[current + 1].getType() == TokenType::NEWLINE) {
        return true;
    }
    
    // Single newline becomes a space
    const Token &newline = advance();
    if (text.isEmpty()) {
        text.append(" ", newline.span().start);
    } else {
        text.appendSpace();
    }
    consumed_any = true;
    return false;
}
std::vector<Document::InlinePtr> Parser::parseInlines(TokenType endToken) {
    std::vector<Document::InlinePtr> inlines;
    TextAccumulator text;
    
    auto flush_text = [&]() {
        if (!text.isEmpty()) {
            inlines.push_back(text.flush(previous().span().end));
        }
    };
    
    while (!isAtEnd() && !check(endToken)) {
        if (check(TokenType::NEWLINE)) {
            // Handle double newline ending paragraph
            if (endToken == TokenType::NEWLINE && 
                current + 1 < tokens_.size() &&
                tokens_[current + 1].getType() == TokenType::NEWLINE) {
                break;
            }
            // Single newline becomes a space
            advance();
            text.appendSpace();
            continue;
        }
        
        if (check(TokenType::STAR)) {
            flush_text();
            inlines.push_back(parseBold());
            continue;
        }
        
        if (check(TokenType::UNDERSCORE)) {
            flush_text();
            inlines.push_back(parseItalic());
            continue;
        }
        
        if (check(TokenType::BACKTICK)) {
            flush_text();
            inlines.push_back(parseCode());
            continue;
        }
        
        // Regular text
        const Token &token = advance();
        text.append(token.getLexeme(), token.span().start);
    }
    
    flush_text();
    return inlines;
}

Document::InlinePtr Parser::parseBold() {
    SourceSpan span;
    span.start = peek().span().start;
    
    advance(); // consume opening *
    
    // Recursively parse content until closing *
    auto children = parseInlines(TokenType::STAR);
    
    if (check(TokenType::STAR)) {
        advance(); // consume closing *
    }
    
    span.end = previous().span().end;
    return Document::Inline::make_bold(std::move(children), span);
}

Document::InlinePtr Parser::parseItalic() {
    SourceSpan span;
    span.start = peek().span().start;
    
    advance(); // consume opening _
    
    auto children = parseInlines(TokenType::UNDERSCORE);
    
    if (check(TokenType::UNDERSCORE)) {
        advance(); // consume closing _
    }
    
    span.end = previous().span().end;
    return Document::Inline::make_italic(std::move(children), span);
}

Document::InlinePtr Parser::parseCode() {
    SourceSpan span;
    span.start = peek().span().start;
    advance(); // consume opening `
    
    // No recursive evaluation inside code blocks
    std::string content;
    while (!isAtEnd() && !check(TokenType::BACKTICK)) {
        const Token &token = advance();
        content += token.getLexeme();
    }
    
    if (check(TokenType::BACKTICK)) {
        advance(); // consume closing `
    }
    
    span.end = previous().span().end;
    return Document::Inline::make_code(std::move(content), span);
}

Document::Block Parser::block() {
  if (check(TokenType::HEADING_MARK))
    return heading();
  return paragraph();
}

const Token &Parser::peek() const { return tokens_.at(current); }

const Token &Parser::previous() const {
  assert(current > 0);
  return tokens_.at(current - 1);
}

const Token &Parser::advance() {
  if (!isAtEnd())
    current++;
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
  if (isAtEnd())
    return false;
  return peek().getType() == type;
}

bool Parser::isAtEnd() { return peek().getType() == TokenType::EOF_; }