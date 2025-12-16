#pragma once

enum class TokenType {
    // Delimiters / symbols
    NEWLINE,
    HASH,       // '#'
    EQUAL,     // '='
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
    EOF_,

};