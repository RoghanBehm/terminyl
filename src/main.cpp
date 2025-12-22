#include "document.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <variant>
#include <fstream>


void write_file(const std::string& path, const std::string& s) {
    std::ofstream f(path);
    f << s;
}

static std::string box_ascii(std::string_view s) {
    const std::size_t w = s.size();
    std::string out;
    out += "+" + std::string(w + 2, '-') + "+\n";
    out += "| " + std::string(s) + " |\n";
    out += "+" + std::string(w + 2, '-') + "+\n";
    return out;
}

std::string render(const Document& doc) {
    std::ostringstream out;

    for (const auto& blk : doc.blocks()) {
        std::visit([&](const auto& b) {
            using T = std::decay_t<decltype(b)>;

            if constexpr (std::is_same_v<T, Document::Heading>) {
                out << box_ascii(b.text) << "\n";
            } else if constexpr (std::is_same_v<T, Document::Paragraph>) {
                out << b.text << "\n\n";
            }
        }, blk);
    }

    return out.str();
}


static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Failed to open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}



int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: typscii [script]\n";
        return 64;
    }

    try {
        std::string source = read_file(argv[1]);
        Lexer lex(source);
        auto tokens = lex.lexTokens();
        auto doc = Parser(std::move(tokens)).parse();
        write_file("out.txt", render(doc));
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
