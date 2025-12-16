#include "token.hpp"

Token::Token(TokenType type, std::string_view lexeme, std::size_t line) : type_(type), lexeme_(lexeme), line_(line) {};





