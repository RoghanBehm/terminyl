#include "token.hpp"

Token::Token(TokenType type, std::string_view lexeme, SourceSpan span,
             Literal value = std::monostate{})
    : type_(type), lexeme_(lexeme), span_(span), value_(value) {};
