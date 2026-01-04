#include "token.hpp"
#include "token_type.hpp"
#include "string"

Token::Token(TokenType type, std::string_view lexeme, SourceSpan span,
             Literal value = std::monostate{})
    : type_(type), lexeme_(lexeme), span_(span), value_(value) {};

std::string Token::to_string(TokenType type) {
  switch (type) {
    using enum TokenType;
  case NEWLINE:
    return "LEFT PAREN";
  case HASH:
    return "HASH";
  case LEFT_PAREN:
    return "LEFT_PAREN"; 
  case RIGHT_PAREN:
    return "RIGHT_PAREN";
  case LEFT_SQ_BRACKET:
    return "LEFT_SQ_BRACKET";
  case RIGHT_SQ_BRACKET:
    return "RIGHT_SQ_BRACKET";
  case COLON:
    return "COLON"; 
  case COMMA:
    return "COMMA";
  case STRING:
    return "STRING";
  case IDENTIFIER:
    return "IDENTIFIER"; 
  case TEXT:
    return "TEXT";
  case HEADING_MARK:
    return "HEADING_MARK";
  case STAR:
    return "STAR";
  case BACKTICK:
    return "BACKTICK"; 
  case UNDERSCORE:
    return "UNDERSCORE"; 
  case EOF_:
    return "EOF_";
  case NUMBER:
    return "NUMBER";
  case BANG:
    return "BANG";
  case BANG_EQUAL:
    return "BANG_EQUAL";
  case EQUAL:
    return "EQUAL";
  case EQUAL_EQUAL:
    return "EQUAL_EQUAL";
  case GREATER:
    return "GREATER";
  case GREATER_EQUAL:
    return "GREATER_EQUAL";
  case LESS:
    return "LESS";
  case LESS_EQUAL:
    return "LESS_EQUAL";
  case PLUS:
    return "PLUS";
  case SLASH:
    return "SLASH";
  case MINUS:
    return "MINUS";
  default:
    return "IDK";
  }
}