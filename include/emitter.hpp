#pragma once
#include <string>
#include "document.hpp"

struct Style {
    std::size_t width = 80;
    std::size_t paragraph_indent = 0;
};

class Emitter {
public:
    explicit Emitter(Style s = {});
    const Style& getStyle() const { return style_; }
    void render(std::ostream& out, const Document& doc) const;
    std::string render_to_string(const Document& doc) const;

private:
    static std::string box_heading(std::string_view text, std::size_t pad = 1);
    Style style_;
};