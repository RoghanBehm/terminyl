#include "lexer.hpp"
#include "token_type.hpp"
#include <cstdio>
#include <iostream>

Lexer::Lexer(std::string &source) : source_(source) {}

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

void Lexer::addToken(TokenType type) {
  std::string_view text{getSource().data() + start, current - start};
  SourceSpan span{start_pos, cur_pos};
  tokens.emplace_back(type, text, span);
}

char Lexer::peek() {
  if (isAtEnd())
    return '\0';
  return getSource().at(current);
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
  case '#':
    addToken(HASH);
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
  case '\n':
    addToken(NEWLINE);
    break;
  case '=':
    if (start_pos.column == 1)
      heading();
    else
      text();
    break;
  default:
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
                      SourceSpan{cur_pos, cur_pos});
    /* DEBUG*/
  for (auto const &t : tokens) {
    std::cout << (int)t.getType() << " '" << t.getLexeme() << "'\n";
  }

  return tokens;
}

void Lexer::text() {
  while (peek() != '\n' && !isAtEnd() && peek() != '*' && peek() != '_' &&
         peek() != '`') {
    advance();
  }

  addToken(TokenType::TEXT);
}