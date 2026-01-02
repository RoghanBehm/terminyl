#include "lexer.hpp"
#include "source.hpp"
#include "token_type.hpp"
#include <cstdio>
#include <iostream>
// #include <iostream>

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
  case '(':
    addToken(LEFT_PAREN);
    break;
  case ')':
    addToken(RIGHT_PAREN);
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
    addToken(HASH);
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
    addToken(NEWLINE);
    break;
  case '=':
    if (start_pos.column == 1)
      heading();
    else
      addToken(match('=') ? EQUAL_EQUAL : EQUAL);
    break;
  default:
    if (isDigit(c))
      number();
    else
      text();
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
  while (!isSpecialChar(peek())) {
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
      return;
    }
    advance();
  }
  if (isAtEnd()) {
    error("Unterminated string", SourceSpan{start_pos, cur_pos});
    return;
  }

  advance();
  std::string_view value{getSource().data() + start + 1, (current - start) - 2};
  addToken(TokenType::STRING, value);
}

bool Lexer::isSpecialChar(char c) {
  static constexpr std::string_view specialChars = "\n*_`#(),+-/=<>!";
  return specialChars.find(c) != std::string_view::npos;
}

bool Lexer::isDigit(char c) { return c >= '0' && c <= '9'; }

void Lexer::error(std::string message, SourceSpan span) {
  diagnostics_.add(Diagnostic(ErrorLevel::Error, std::move(message), span));
}
