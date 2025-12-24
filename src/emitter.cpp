#include "emitter.hpp"
#include <variant>
#include <iostream>
#include <sstream>

Emitter::Emitter(Style s) : style_(std::move(s)) {}

void Emitter::render(std::ostream& out, const Document& doc) const {
    for (const auto& blk : doc.blocks()) {
        std::visit([&](const auto& b) {
            using T = std::remove_cvref_t<decltype(b)>;

            if constexpr (std::is_same_v<T, Document::Heading>) {
                out << box_heading(b.text) << "\n";
            } else if constexpr (std::is_same_v<T, Document::Paragraph>) {
                wrap_paragraph(out, b.text, style_.width, style_.paragraph_indent);
                out << "\n"; 
            }
        }, blk);
    }
}

std::string Emitter::render_to_string(const Document& doc) const {
    std::ostringstream out;
    render(out, doc);
    return out.str();
}


std::string Emitter::box_heading(std::string_view s, std::size_t pad) {
    const std::size_t w = s.size();
    const std::size_t inner = w + 2 * pad;

    std::string out;
    out.reserve((inner + 4) * 3);

    out += '+';
    out += std::string(inner + 2, '-');
    out += "+\n";

    out += '|';
    out += std::string(pad + 1, ' ');
    out += std::string(s);
    out += std::string(pad + 1, ' ');
    out += "|\n";

    out += '+';
    out += std::string(inner + 2, '-');
    out += "+\n";

    return out;
}

void Emitter::skip_whitespace(std::size_t& i, std::string_view text) const {
    while (i < text.size() && 
    std::isspace(static_cast<unsigned char>(text[i])))
        ++i;
}

void Emitter::skip_non_whitespace(std::size_t& i, std::string_view text) const {
    while (i < text.size() &&
    !std::isspace(static_cast<unsigned char>(text[i])))
        ++i;
}



void Emitter::wrap_paragraph(std::ostream& out, std::string_view text, std::size_t width, std::size_t indent) const {
    std::size_t line_len = 0;

    // add n many blank chars, where n is the indent var
    auto write_indent = [&]() {
        out << std::string(indent, ' ');
        line_len = indent;
    };

    write_indent();

    std::size_t i = 0;
    while (i < text.size()) {
        skip_whitespace(i, text);
        
        if (i >= text.size()) break;



        std::size_t start = i;
        skip_non_whitespace(i, text);
        std::string_view word = text.substr(start, i - start);

        if (line_len == indent) {
            out << word;
            line_len += word.size();
        } else if (line_len + 1 + word.size() <= width) {
            out << ' ' << word;
            line_len += 1 + word.size();
        } else {
            out << '\n';
            write_indent();
            out << word;
            line_len += word.size();
        }
    }

    out << '\n';
}