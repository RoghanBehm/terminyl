#include "emitter.hpp"
#include "io.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <iostream>
#include <string>



int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: terminyl [script]\n";
        return 64;
    }

    try {
        std::string source = read_file(argv[1]);
        Lexer lex(source);
        auto tokens = lex.lexTokens();
        auto doc = Parser(std::move(tokens)).parse();
        
        Emitter emitter;
        emitter.render(std::cout, doc);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
