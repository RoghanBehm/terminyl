#pragma once

#include <cstdint>
struct SourcePos {
    std::uint32_t line = 1;
    std::uint32_t column = 1;
};

struct SourceSpan {
    SourcePos start;
    SourcePos end;
};