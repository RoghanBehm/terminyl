#include "emitter.hpp"
#include "lowerer.hpp"
#include "io.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <iostream>
#include <string>



int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: terminyl [file]\n";
        return 64;
    }

    try {
        std::string source = read_file(argv[1]);
        Lexer lex(source);
        auto tokens = lex.lexTokens();
        auto doc = Parser(std::move(tokens)).parse();
        
        Lowerer lowerer(doc);
        auto low_doc = lowerer.lower();
        Emitter emitter;
        emitter.render(std::cout, low_doc);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
