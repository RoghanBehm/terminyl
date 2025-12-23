#include "emitter.hpp"
#include <variant>
#include <iostream>
#include <sstream>

void Emitter::render(std::ostream& out, const Document& doc) const {
    for (const auto& blk : doc.blocks()) {
        std::visit([&](const auto& b) {
            using T = std::remove_cvref_t<decltype(b)>;

            if constexpr (std::is_same_v<T, Document::Heading>) {
                out << box_heading(b.text) << "\n";
            } else if constexpr (std::is_same_v<T, Document::Paragraph>) {
                out << b.text << "\n\n";
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
