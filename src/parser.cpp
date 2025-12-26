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
    
    TextAccumulator text;
    bool consumed_any = false;
    
    auto flush_text = [&]() {
        if (!text.isEmpty()) {
            para.inlines.push_back(text.flush(previous().span().end));
        }
    };
    
    while (!isAtEnd()) {
        if (handleNewlineInParagraph(text, consumed_any)) {
            break; // Double newline ends paragraph
        }
        
        if (check(TokenType::STAR)) {
        flush_text(); 
        handleEmphasis(para, text, consumed_any);
            continue;
        }
        
        // Regular text token
        const Token &token = advance();
        text.append(token.getLexeme(), token.span().start);
        consumed_any = true;
    }
    
    flush_text();
    
    // Skip trailing newlines
    while (match(TokenType::NEWLINE)) {}
    
    para.span.end = consumed_any ? previous().span().end : peek().span().start;
    return para;
}

// Returns true if paragraph should end
bool Parser::handleNewlineInParagraph(TextAccumulator& text, bool& consumed_any) {
    if (!check(TokenType::NEWLINE)) {
        return false;
    }
    
    // Check for double newline (paragraph break)
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

// Returns true if emphasis was successfully parsed
bool Parser::handleEmphasis(Document::Paragraph& para, TextAccumulator& text, 
                           bool& consumed_any) {
    const Token &openStar = advance();
    consumed_any = true;
    
    std::string emph_text;
    while (!isAtEnd() && !check(TokenType::STAR) && !check(TokenType::NEWLINE)) {
        emph_text += std::string(advance().getLexeme());
        consumed_any = true;
    }
    
    if (check(TokenType::STAR)) {
        const Token &closeStar = advance();
        consumed_any = true;
        
        SourceSpan emphSpan{openStar.span().start, closeStar.span().end};
        para.inlines.push_back(Document::Inline::make_emph(
            {Document::Inline::make_text(std::move(emph_text), emphSpan)},
            emphSpan));
        return true;
    }
    
    // Unclosed emphasis - treat as literal text
    text.append("*", openStar.span().start);
    text.append(emph_text, openStar.span().start);
    return false;
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