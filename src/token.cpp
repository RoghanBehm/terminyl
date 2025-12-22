#include "token.hpp"

Token::Token(TokenType type, std::string_view lexeme, SourceSpan span) : type_(type), lexeme_(lexeme), span_(span) {};



