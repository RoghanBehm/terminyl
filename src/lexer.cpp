#include "lexer.hpp"
#include "source.hpp"
#include "token_type.hpp"
#include <cstdio>

Lexer::Lexer(const std::string &source) : source_(source) {}

bool Lexer::isAtEnd() { return current >= getSource().length(); }

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

void Lexer::addToken(TokenType type) { addToken(type, std::monostate{}); }

void Lexer::addToken(TokenType type, Literal value) {
  std::string_view text{getSource().data() + start, current - start};
  tokens.emplace_back(type, text, SourceSpan{start_pos, cur_pos}, value);
}

char Lexer::peek() {
  if (isAtEnd())
    return '\0';
  return getSource().at(current);
}

char Lexer::peekNext() {
  if (current + 1 >= getSource().length())
    return '\0';
  return getSource().at(current + 1);
}

bool Lexer::match(char expected) {
  if (isAtEnd())
    return false;
  if (getSource().at(current) != expected)
    return false;

  advance();
  return true;
}

void Lexer::lexToken() {
  using enum TokenType;
  char c = advance();
  switch (c) {
  case ' ':
  case '\t':
  case '\r':
    // Only skip whitespace in expression mode
    if (mode_ == LexerMode::EXPRESSION) {
      return;
    }
    // In text mode, back up and let text() handle it
    current--;
    cur_pos.column--;
    text();
    return;
  case '(':
    if (mode_ == LexerMode::EXPRESSION) {
      paren_depth_++;
      addToken(TokenType::LEFT_PAREN);
    } else {
      // treat as normal text
      current--;
      cur_pos.column--;
      text();
    }
    break;
  case ')':
    if (mode_ == LexerMode::EXPRESSION) {
      addToken(TokenType::RIGHT_PAREN);
      paren_depth_--;
      if (paren_depth_ == 0)
        mode_ = LexerMode::TEXT;
    } else {
      current--;
      cur_pos.column--;
      text();
    }
    break;
  case '[':
    addToken(LEFT_SQ_BRACKET);
    break;
  case ']':
    addToken(RIGHT_SQ_BRACKET);
    break;
  case ',':
    addToken(COMMA);
    break;
  case '+':
    addToken(PLUS);
    break;
  case '/':
    addToken(SLASH);
    break;
  case '#':
    if (peek() == '(') {
      advance();
      paren_depth_ = 1;
      mode_ = LexerMode::EXPRESSION;
      addToken(TokenType::HASH);
      addToken(TokenType::LEFT_PAREN);
    } else {
      addToken(TokenType::HASH);
    }
    break;
  case '"':
    string();
    break;
  case '*':
    addToken(STAR);
    break;
  case '`':
    addToken(BACKTICK);
    break;
  case '_':
    addToken(UNDERSCORE);
    break;
  case '-':
    addToken(MINUS);
    break;
  case '!':
    addToken(match('=') ? BANG_EQUAL : BANG);
    break;
  case '<':
    addToken(match('=') ? LESS_EQUAL : LESS);
    break;
  case '>':
    addToken(match('=') ? GREATER_EQUAL : GREATER);
    break;
  case '\n':
    if (peek() == '\n') {
      advance();

      while (peek() == '\n') {
        advance();
      }
      addToken(NEWLINE);
    }
    break;
  case '=':
    if (start_pos.column == 1)
      heading();
    else
      addToken(match('=') ? EQUAL_EQUAL : EQUAL);
    break;
  default:
    if (mode_ == LexerMode::EXPRESSION) {
      if (isDigit(c))
        number();
      else if (isAlpha(c))
        identifier();
      else
        error("Unknown expression", SourceSpan{start_pos, cur_pos});
    } else {
      current--;
    cur_pos.column--;
    text();
    }
    break;
  }
}

void Lexer::heading() {
  while (!isAtEnd() && getSource().at(current) == '=')
    advance();

  addToken(TokenType::HEADING_MARK);
  // can determine heading lvl with token.lexeme_.size();
}

std::vector<Token> Lexer::lexTokens() {
  while (!isAtEnd()) {
    start = current;
    start_pos = cur_pos;
    lexToken();
  }

  tokens.emplace_back(TokenType::EOF_, std::string_view{},
                      SourceSpan{cur_pos, cur_pos}, std::monostate{});

  /* DEBUG
  for (auto const &t : tokens) {
    std::cout << Token::to_string(t.getType()) << " '" << t.getLexeme()
              << "'\n";
  }
*/
  return tokens;
}

void Lexer::text() {
  while (!isAtEnd() && !isSpecialChar(peek())) {
    advance();
  }
  addToken(TokenType::TEXT);
}

void Lexer::number() {
  while (isDigit(peek()))
    advance();
  if (peek() == '.' && isDigit(peekNext())) {
    advance();
    while (isDigit(peek()))
      advance();
  }

  auto value = getSource().substr(start, current - start);
  addToken(TokenType::NUMBER, std::stod(value));
}

void Lexer::string() {
  while (!isAtEnd() && peek() != '"') {
    if (peek() == '\n') {
      error("Unterminated string", SourceSpan{start_pos, cur_pos});
      addToken(TokenType::STRING, std::string_view{});
      return;
    }
    advance();
  }

  if (isAtEnd()) {
    error("Unterminated string", SourceSpan{start_pos, cur_pos});
    addToken(TokenType::STRING, std::string_view{});
    return;
  }

  advance();
  std::string_view value{getSource().data() + start + 1, (current - start) - 2};
  addToken(TokenType::STRING, value);
}

bool Lexer::isSpecialChar(char c) {
  if (mode_ == LexerMode::EXPRESSION) {
    static constexpr std::string_view exprSpecial = "\n*_`#(),+-/=<>!";
    return exprSpecial.find(c) != std::string_view::npos;
  } else {
    static constexpr std::string_view textSpecial = "\n*_`#";
    return textSpecial.find(c) != std::string_view::npos;
  }
}

bool Lexer::isDigit(char c) { return c >= '0' && c <= '9'; }

bool Lexer::isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }

void Lexer::identifier() {
  while (isAlphaNumeric(peek()))
    advance();

  std::string_view text{source_.data() + start,
                        static_cast<size_t>(current - start)};
  auto it = keywords.find(text);
  TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
  addToken(type);
}

void Lexer::error(std::string message, SourceSpan span) {
  diagnostics_.add(Diagnostic(ErrorLevel::Error, std::move(message), span));
}