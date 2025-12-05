#pragma once

#include <string>
#include "source.hpp"
#include "token_type.hpp"

struct Token {
    TokenType type;
    std::string lexeme;
    SourceSpan span;
};
