#pragma once

#include <cstdint>

enum class TokenType : std::uint8_t {
    // Delimiters / symbols
    NEWLINE,
    HASH,       // '#'
    LEFT_PAREN,     // '('
    RIGHT_PAREN,     // ')'
    LEFT_SQ_BRACKET,   // '['
    RIGHT_SQ_BRACKET,   // ']'
    COLON,      // ':'
    COMMA,      // ','
    STRING,     // "hi"
    IDENTIFIER,      // style, box, grid, dither
    TEXT,       // free text chunks (for paragraphs/headings)
    HEADING_MARK,
    STAR, // '*'
    BACKTICK, // '`'
    UNDERSCORE, // '_'
    EOF_,
    NUMBER,


    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Operators
    PLUS, SLASH,
    MINUS,

};




