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

void Lexer::addToken(TokenType type, std::string_view lexeme, Literal value) {
  tokens.emplace_back(type, lexeme, SourceSpan{start_pos, cur_pos}, value);
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
    // Only skip whitespace in expression/block mode
    if (mode_ == LexerMode::EXPRESSION || mode_ == LexerMode::BLOCK)
      return;
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
      // Only exit to TEXT mode if expression was triggered by #( and we're back
      // to depth 0
      if (paren_depth_ == 0 && paren_triggered_expr_) {
        mode_ = LexerMode::TEXT;
        paren_triggered_expr_ = false;
      }
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
    if (match('/')) {
      while (peek() != '\n' && !isAtEnd())
        advance();
    } else if (match('*')) {
      int nesting = 1;
      while (!isAtEnd()) {
        if (peek() == '/' && peekNext() == '*') {
          nesting++;
          advance();
          advance();
        } else if (peek() == '*' && peekNext() == '/') {
          nesting--;
          advance();
          advance();
          if (nesting == 0)
            break;
        } else {
          advance();
        }
      }
      if (nesting > 0) {
        error("Unterminated block comment", SourceSpan{start_pos, cur_pos});
      }
    } else {
      addToken(SLASH);
    }
    break;
  case '#':
    addToken(TokenType::HASH);

    if (!in_code_span_) {
      // Temporarily enter expression mode to lex the next token
      if (isAlpha(peek())) {
        mode_ = LexerMode::EXPRESSION;
        paren_triggered_expr_ = false;

        start = current;
        start_pos = cur_pos;
        identifier();

        auto t = tokens.back().getType();
        if (t == TokenType::WHILE) {
          pending_block_directive_ = true;
        }

        auto ty = tokens.back().getType();
        if (ty != TokenType::LET && ty != TokenType::FN &&
            ty != TokenType::WHILE && peek() != '(') {
          mode_ = LexerMode::TEXT;
        }

      } else if (peek() == '(') {
        advance();
        paren_depth_ = 1;
        paren_triggered_expr_ = true;
        mode_ = LexerMode::EXPRESSION;
        addToken(LEFT_PAREN);
      }
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
    in_code_span_ = !in_code_span_;
    break;
  case '_':
    addToken(UNDERSCORE);
    break;
  case '{':

    addToken(TokenType::LEFT_BRACE);
    if (pending_block_directive_) {
      pending_block_directive_ = false;
      mode_ = LexerMode::BLOCK;
    }
    break;
  case '}':
    addToken(TokenType::RIGHT_BRACE);
    if (mode_ == LexerMode::BLOCK) {
      mode_ = LexerMode::TEXT;
    }
    break;

  case '-':
    addToken(MINUS);
    break;
  case '&':
    if (mode_ == LexerMode::EXPRESSION) {
      if (match('&'))
        addToken(TokenType::AND);
      else
        error("Unexpected '&' (did you mean '&&'?)",
              SourceSpan{start_pos, cur_pos});
    } else {
      current--;
      cur_pos.column--;
      text();
    }
    break;

  case '|':
    if (mode_ == LexerMode::EXPRESSION) {
      if (match('|'))
        addToken(TokenType::OR);
      else
        error("Unexpected '|' (did you mean '||'?)",
              SourceSpan{start_pos, cur_pos});
    } else {
      current--;
      cur_pos.column--;
      text();
    }
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
    if (mode_ == LexerMode::EXPRESSION && paren_depth_ == 0) {
      mode_ = LexerMode::TEXT;
    }

    if (mode_ == LexerMode::BLOCK) {
      while (peek() == '\n')
        advance();
      addToken(TokenType::NEWLINE);
      break;
    }

    if (peek() == '\n') {
      advance();

      while (peek() == '\n') {
        advance();
      }
      addToken(NEWLINE);
      break;
    }

    if (mode_ == LexerMode::TEXT) {
      addToken(SPACE);
    }

    break;
  case '=':
    if (start_pos.column == 1)
      heading();
    else
      addToken(match('=') ? EQUAL_EQUAL : EQUAL);
    break;
  default:
    if (mode_ == LexerMode::EXPRESSION || mode_ == LexerMode::BLOCK) {
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
    // Character escape check
    if (peek() == '\\' && current + 1 < source_.size()) {
      advance();
      if (!isAtEnd()) {
        advance();
      }
    } else {
      advance();
    }
  }
  std::string_view raw{getSource().data() + start, current - start};
  if (raw.find('\\') != std::string_view::npos) {
    std::string processed = processEscapes(raw);
    std::string_view value = storeProcessed(std::move(processed));
    addToken(TokenType::TEXT, value, std::monostate{});
  } else {
    addToken(TokenType::TEXT);
  }
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

    if (peek() == '\\' && peekNext() != '\0') {
      advance();
      advance();
    } else {
      advance();
    }
  }

  if (isAtEnd()) {
    error("Unterminated string", SourceSpan{start_pos, cur_pos});
    addToken(TokenType::STRING, std::string_view{});
    return;
  }

  advance();

  std::string_view raw{getSource().data() + start + 1, (current - start) - 2};

  if (raw.find('\\') != std::string_view::npos) {
    std::string processed = processEscapes(raw);
    std::string_view value = storeProcessed(std::move(processed));
    addToken(TokenType::STRING, value, value);
  } else {
    addToken(TokenType::STRING, raw, raw);
  }
}

bool Lexer::isSpecialChar(char c) {
  if (mode_ == LexerMode::EXPRESSION) {
    static constexpr std::string_view exprSpecial = "\n*_`#(),+-/=<>!&|";
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

  std::string_view text{source_.data() + start, (current - start)};
  auto it = keywords.find(text);
  TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
  addToken(type);
}

void Lexer::error(std::string message, SourceSpan span) {
  diagnostics_.add(Diagnostic(ErrorLevel::Error, std::move(message), span));
}

std::string Lexer::processEscapes(std::string_view raw) {
  std::string result;
  result.reserve(raw.size());

  for (size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\\' && i + 1 < raw.size()) {
      char next = raw[i + 1];

      switch (next) {
      // Markup escapes (remove backslash)
      case '*':
        result += '*';
        i++;
        break;
      case '_':
        result += '_';
        i++;
        break;
      case '`':
        result += '`';
        i++;
        break;
      case '#':
        result += '#';
        i++;
        break;
      case '\\':
        result += '\\';
        i++;
        break;

      // Standard escapes
      case 'n':
        result += '\n';
        i++;
        break;
      case 't':
        result += '\t';
        i++;
        break;
      case 'r':
        result += '\r';
        i++;
        break;
      case '"':
        result += '"';
        i++;
        break;

      default:
        // Unknown escape
        result += '\\';
        break;
      }
    } else {
      result += raw[i];
    }
  }

  return result;
}