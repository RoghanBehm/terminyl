#pragma once

struct SourcePos {
    int line = 0;
    int column = 0;
};

struct SourceSpan {
    SourcePos start;
    SourcePos end;
};