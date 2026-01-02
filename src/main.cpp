#include "emitter.hpp"
#include "lowerer.hpp"
#include "io.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "error.hpp"
#include <iostream>
#include <string>



int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: terminyl [file]\n";
        return 64;
    }

    try {
        SourceFile src{argv[1], read_file(argv[1])};
        Lexer lex(src.content);
        auto tokens = lex.lexTokens();

        if (lex.diagnostics().has_errors()) {
            lex.diagnostics().report_all(src);
            return 1;
        }

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
