#include "parser.hpp"
#include "source.hpp"
#include "text_accumulator.hpp"
#include "token_type.hpp"
#include <cassert>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

using Expr = Document::Expr;

Document Parser::parse() {
  Document doc;

  while (!isAtEnd()) {
    skipBlanks();
    if (isAtEnd())
      break;

    doc.add(block());
  }
  return doc;
}

Document::Expr::Ptr
Parser::laparse(const std::function<Document::Expr::Ptr()> &op_type,
                std::initializer_list<TokenType> types, const buildFn &build) {
  Document::Expr::Ptr lhs = op_type();

  while (match(types)) {
    Token const &op = previous();
    Document::Expr::Ptr rhs = op_type();
    SourceSpan sp{lhs->span.start, rhs->span.end};
    lhs = build(std::move(lhs), op, std::move(rhs), sp);
  }
  return lhs;
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

  if (check(TokenType::NEWLINE)) {
    advance();
  }

  para.span.end = previous().span().end;
  return para;
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
      break;
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

    if (check(TokenType::HASH)) {
      flush_text();
      advance(); // '#'

      if (check(TokenType::LET)) {
        // #let name = value
        inlines.push_back(parseLet());
      } else if (check(TokenType::LEFT_PAREN)) {
        // #(expr)
        inlines.push_back(parseSplice());
      } else if (check(TokenType::IDENTIFIER)) {
        // #name
        inlines.push_back(parseVarReference());
      } else {
        error("Expected 'let', '(', or identifier after '#'", peek().span());
      }
      continue;
    }
    // Regular text
    const Token &token = advance();

    text.append(token.getLexeme(), token.span().start);
  }

  if (isAtEnd() && endToken != TokenType::EOF_ &&
      endToken != TokenType::NEWLINE) {
    // Warn about unclosed
    error("Unclosed formatting (expected closing delimiter)",
          previous().span());
  }

  flush_text();
  return inlines;
}

Document::InlinePtr Parser::parseBold() {
  SourceSpan span;
  span.start = peek().span().start;

  SourceSpan opening_span = peek().span();

  advance(); // consume opening *

  auto children = parseInlines(TokenType::STAR);

  if (!check(TokenType::STAR)) {
    error("Expected closing '*'", opening_span);
  } else {
    advance(); // consume closing *
  }

  span.end = previous().span().end;
  return Document::Inline::make_bold(std::move(children), span);
}

Document::InlinePtr Parser::parseItalic() {
  SourceSpan span;
  span.start = peek().span().start;

  SourceSpan opening_span = peek().span();

  advance(); // consume opening _

  auto children = parseInlines(TokenType::UNDERSCORE);

  if (!check(TokenType::UNDERSCORE)) {
    error("Expected closing '_'", opening_span);
  } else {
    advance(); // consume closing _
  }

  span.end = previous().span().end;
  return Document::Inline::make_italic(std::move(children), span);
}

Document::InlinePtr Parser::parseCode() {
  SourceSpan span;
  span.start = peek().span().start;

  SourceSpan opening_span = peek().span();

  advance(); // consume opening `

  // No recursive evaluation inside code blocks
  std::string content;
  while (!isAtEnd() && !check(TokenType::BACKTICK)) {
    const Token &token = advance();
    content += token.getLexeme();
  }

  if (!check(TokenType::BACKTICK)) {
    error("Expected closing '`'", opening_span);
  } else {
    advance(); // consume closing `
  }

  span.end = previous().span().end;
  return Document::Inline::make_code(std::move(content), span);
}

Document::InlinePtr Parser::parseVarReference() {
  SourceSpan span;
  span.start = previous().span().start;
  if (!check(TokenType::IDENTIFIER)) {
    error("Expected identifier after '#'", peek().span());
    return Document::Inline::make_text("[ERROR]", span);
  }

  std::string name = std::string(peek().getLexeme());
  advance(); // identifier
   if (check(TokenType::LEFT_PAREN)) {
    advance(); // '('
    
    std::vector<Document::ExprPtr> args;
    if (!check(TokenType::RIGHT_PAREN)) {
      do {
        args.push_back(expression());
      } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
    span.end = previous().span().end;
    
    auto call_expr = Document::Expr::make_call(std::move(name), std::move(args), span);
    return Document::Inline::make_splice(std::move(call_expr), span);
  }

  span.end = previous().span().end;

  auto var_expr = Document::Expr::make_var(std::move(name), span);
  return Document::Inline::make_splice(std::move(var_expr), span);
}

Document::InlinePtr Parser::parseLet() {
  SourceSpan span;
  span.start = previous().span().start;

  consume(TokenType::LET, "Expected 'let'");

  if (!check(TokenType::IDENTIFIER)) {
    error("Expected variable name after 'let'", peek().span());
    return Document::Inline::make_text("[ERROR]", span);
  }

  std::string name = std::string(peek().getLexeme());
  advance(); // consume ident

  consume(TokenType::EQUAL, "Expected '=' in let binding");

  Document::ExprPtr value = expression();

  span.end = previous().span().end;
  return Document::Inline::make_let(std::move(name), std::move(value), span);
}

Document::InlinePtr Parser::parseSplice() {
  SourceSpan span;
  span.start = peek().span().start;

  if (!check(TokenType::LEFT_PAREN)) {
    error("Expected '(' after '#'", peek().span());
    return Document::Inline::make_text("[ERROR]", span);
  }
  advance(); // '('

  if (check(TokenType::RIGHT_PAREN)) {
    error("Empty expression", peek().span());
    advance();
    span.end = previous().span().end;
    return Document::Inline::make_text("0", span);
  }

  Document::ExprPtr e = expression(); // start from top

  if (!check(TokenType::RIGHT_PAREN)) {
    error("Expected ')' after expression", peek().span());
    synchronize();
  } else {
    advance();
  }

  span.end = previous().span().end;
  return Document::Inline::make_splice(std::move(e), span);
}

Document::ExprPtr Parser::expression() { return logical_or(); }

Document::Expr::Ptr Parser::logical_or() {
  return laparse([this] { return logical_and(); }, {TokenType::OR},
                 [](auto lhs, Token const &op, auto rhs, SourceSpan sp) {
                   return Expr::make_logical(std::move(lhs), op, std::move(rhs),
                                             sp);
                 });
}

Document::Expr::Ptr Parser::logical_and() {
  return laparse([this] { return equality(); }, {TokenType::AND},
                 [](auto lhs, Token const &op, auto rhs, SourceSpan sp) {
                   return Expr::make_logical(std::move(lhs), op, std::move(rhs),
                                             sp);
                 });
}

Document::Expr::Ptr Parser::equality() {
  return laparse([this] { return comparison(); },
                 {TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL},
                 [](auto lhs, Token const &op, auto rhs, SourceSpan sp) {
                   return Document::Expr::make_binary(std::move(lhs), op,
                                                      std::move(rhs), sp);
                 });
}

Document::Expr::Ptr Parser::comparison() {
  return laparse([this]() { return term(); },
                 {TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS,
                  TokenType::LESS_EQUAL},
                 [](auto lhs, Token const &op, auto rhs, SourceSpan sp) {
                   return Document::Expr::make_binary(std::move(lhs), op,
                                                      std::move(rhs), sp);
                 });
}

Document::Expr::Ptr Parser::term() {
  return laparse([this] { return factor(); },
                 {TokenType::MINUS, TokenType::PLUS},
                 [](auto lhs, Token const &op, auto rhs, SourceSpan sp) {
                   return Document::Expr::make_binary(std::move(lhs), op,
                                                      std::move(rhs), sp);
                 });
}

Document::Expr::Ptr Parser::factor() {
  return laparse([this]() { return unary(); },
                 {TokenType::SLASH, TokenType::STAR},
                 [](auto lhs, Token const &op, auto rhs, SourceSpan sp) {
                   return Document::Expr::make_binary(std::move(lhs), op,
                                                      std::move(rhs), sp);
                 });
}

Document::Expr::Ptr Parser::unary() {
  if (match({TokenType::BANG, TokenType::MINUS})) {
    Token op = previous();
    Document::Expr::Ptr right = unary();
    SourceSpan sp;
    sp.start = op.span().start;
    sp.end = right->span.end;
    return Document::Expr::make_unary(op, std::move(right), sp);
  }

  return primary();
}

Document::ExprPtr Parser::primary() {

  if (match(TokenType::TRUE))
    return Document::Expr::make_bool(true, previous().span());
  if (match(TokenType::FALSE))
    return Document::Expr::make_bool(false, previous().span());

  if (match(TokenType::NONE)) {
    return Document::Expr::make_none(previous().span());
  }

  if (match(TokenType::NUMBER)) {
    const Token &t = previous();
    return Expr::make_num(std::get<double>(t.getLiteral()), t.span());
  }

  if (match(TokenType::STRING)) {
    const Token &t = previous();
    std::string value = std::string(t.getLexeme());
    return Document::Expr::make_str(value, t.span());
  }

  if (match(TokenType::IDENTIFIER)) {
    const Token &t = previous();
    std::string name = std::string(t.getLexeme());


    if (match(TokenType::LEFT_PAREN)) {
      return call(name);
    }
    
    // Var reference
    return Document::Expr::make_var(name, t.span());
  }

  if (match(TokenType::LEFT_PAREN)) {
    Document::ExprPtr e = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' in expression\n");
    return e;
  }

  error("Expected literal or '('", peek().span());
  synchronize();
  return Document::Expr::make_num(0.0, previous().span()); // Error placeholder
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

bool Parser::match(std::initializer_list<TokenType> types) {
  for (auto &t : types) {
    if (check(t)) {
      advance();
      return true;
    }
  }
  return false;
}

bool Parser::check(TokenType type) {
  if (isAtEnd())
    return false;
  return peek().getType() == type;
}

bool Parser::isAtEnd() { return peek().getType() == TokenType::EOF_; }

Token Parser::consume(TokenType type, std::string message) {
  if (check(type))
    return advance();
  error(std::move(message), peek().span());
  return advance();
}

void Parser::synchronize() {
  while (!isAtEnd()) {
    // Newline means safe sync point
    if (previous().getType() == TokenType::NEWLINE)
      return;

    switch (peek().getType()) {
    case TokenType::HEADING_MARK:
    case TokenType::HASH:
      return;
    default:
      advance();
    }
  }
}

void Parser::error(std::string message, SourceSpan span) {
  diagnostics_.add(Diagnostic(ErrorLevel::Error, std::move(message), span));
}

Document::ExprPtr Parser::call(std::string name) {
  SourceSpan span;
  span.start = previous().span().start; // '('
  
  std::vector<Document::ExprPtr> args;
  
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      args.push_back(expression());
    } while (match(TokenType::COMMA));
  }
  
  consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
  span.end = previous().span().end;
  
  return Document::Expr::make_call(std::move(name), std::move(args), span);
}