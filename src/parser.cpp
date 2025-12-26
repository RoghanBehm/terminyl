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

  Document::Paragraph p;
  p.span.start = peek().span().start;

  std::string text;
  SourcePos text_start{};
  bool text_has_start = false;
  bool consumed_any = false;

  auto flush_text = [&]() {
    if (text.empty())
      return;
    SourceSpan sp;
    sp.start = text_start;
    sp.end =
        previous().span().end; // last consumed token that contributed to `text`
    p.inlines.push_back(Document::Inline::make_text(std::move(text), sp));
    text.clear();
    text_has_start = false;
  };

  while (!isAtEnd()) {
    if (check(TokenType::NEWLINE)) {
      // End paragraph on double newline or newline + EOF
      if (current + 1 < tokens_.size() &&
          tokens_[current + 1].getType() == TokenType::NEWLINE) {
        break;
      }
      // Treat single newline as space
      const Token &nl = advance();
      if (text.empty()) {
        text_start = nl.span().start;
        text_has_start = true;
      }
      if (!text.empty() && text.back() != ' ')
        text.push_back(' ');
      consumed_any = true;
      continue;
    }

    if (check(TokenType::STAR)) {

      flush_text();

      const Token &openStar = advance();
      consumed_any = true;

      std::string emph_acc;
      while (!isAtEnd() && !check(TokenType::STAR) &&
             !check(TokenType::NEWLINE)) {
        emph_acc += std::string(advance().getLexeme());
        consumed_any = true;
      }

      if (check(TokenType::STAR)) {
        const Token &closeStar = advance();
        consumed_any = true;

        SourceSpan emphSpan;
        emphSpan.start = openStar.span().start;
        emphSpan.end = closeStar.span().end;

        p.inlines.push_back(Document::Inline::make_emph(
            {Document::Inline::make_text(std::move(emph_acc), emphSpan)},
            emphSpan));
        continue;
      } else {
        if (!text_has_start) {
          text_start = openStar.span().start;
          text_has_start = true;
        }
        text += "*";
        text += emph_acc;
        continue;
      }
    }

    const Token &t = advance();
    if (!text_has_start) {
      text_start = t.span().start;
      text_has_start = true;
    }
    text += std::string(t.getLexeme());
    consumed_any = true;
  }

  flush_text();

  while (match(TokenType::NEWLINE)) {
  }

  if (consumed_any) {
    p.span.end = previous().span().end;
  } else {
    p.span.end = peek().span().start;
  }

  return p;
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