#pragma once
#include <string>
#include "document.hpp"

class TextAccumulator {
private:
    std::string text_;
    SourcePos start_;
    bool has_start_ = false;

public:
    void append(std::string_view lexeme, SourcePos pos) {
        if (!has_start_) {
            start_ = pos;
            has_start_ = true;
        }
        text_ += lexeme;
    }
    
    void appendSpace() {
        if (!text_.empty() && text_.back() != ' ') {
            text_.push_back(' ');
        }
    }
    
    bool isEmpty() const { return text_.empty(); }
    
    bool hasStart() const { return has_start_; }
    
    SourcePos startPos() const { return start_; }
    
    Document::Inline::Ptr flush(SourcePos end_pos) {
        SourceSpan span{start_, end_pos};
        auto result = Document::Inline::make_text(std::move(text_), span);
        text_.clear();
        has_start_ = false;
        return result;
    }
};