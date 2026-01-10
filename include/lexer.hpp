#pragma once

#include "error.hpp"
#include "token.hpp"
#include "token_type.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class Lexer {
public:
  explicit Lexer(const std::string &source);

  Token next();
  char peek();
  bool match(char expected);
  std::vector<Token> scan_tokens();
  void addToken(TokenType type);
  void addToken(TokenType type, Literal value);
  void addToken(TokenType type, std::string_view lexeme, Literal value);
  void lexToken();
  void heading();
  std::vector<Token> lexTokens();
  const std::string &getSource() const { return source_; }
  const DiagnosticSet &diagnostics() const { return diagnostics_; }

private:
  enum class LexerMode : std::uint8_t {
    TEXT,       // Normal text mode: spaces significant
    EXPRESSION, // Expression mode: skip whitespace
  };

  std::vector<std::string> string_pool_;

  // String processing without storing strings
  std::string_view storeProcessed(std::string processed) {
    string_pool_.push_back(std::move(processed));
    return string_pool_.back();
  }

  std::string processEscapes(std::string_view raw);

  char peekNext();
  char advance();
  void skip_spaces();
  void text();
  Token ident_or_text();
  Token punctuation();
  void number();
  void string();
  bool isAtEnd();
  bool isDigit(char c);
  bool isAlpha(char c);
  bool isAlphaNumeric(char c);
  void identifier();
  bool isSpecialChar(char c);
  const std::string &source_;
  std::size_t start = 0;
  std::size_t current = 0;
  SourcePos start_pos{1, 1};
  SourcePos cur_pos{1, 1};
  std::vector<Token> tokens;
  DiagnosticSet diagnostics_;
  LexerMode mode_ = LexerMode::TEXT;
  int paren_depth_ = 0;
  bool in_code_span_ = false;
  bool paren_triggered_expr_ = false; // Track if #( triggered expression mode
  void error(std::string message, SourceSpan span);

  // Reserved words
  inline static const std::unordered_map<std::string_view, TokenType> keywords =
      {{"true", TokenType::TRUE}, {"false", TokenType::FALSE},
       {"let", TokenType::LET},   {"if", TokenType::IF},
       {"else", TokenType::ELSE}, {"fn", TokenType::FN},
       {"none", TokenType::NONE}};
};