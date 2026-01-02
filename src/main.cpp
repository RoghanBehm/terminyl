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
        
        // Lex
        Lexer lexer(src.content);
        auto tokens = lexer.lexTokens();
        
        // Parse
        Parser parser(std::move(tokens));
        auto doc = parser.parse();
        
        // Lower
        Lowerer lowerer(doc);
        auto low_doc = lowerer.lower();
        
        // Error reporting
        bool has_errors = false;
        
        if (lexer.diagnostics().has_errors()) {
            lexer.diagnostics().report_all(src);
            has_errors = true;
        }
        
        if (parser.diagnostics().has_errors()) {
            parser.diagnostics().report_all(src);
            has_errors = true;
        }
        
        /* TO BE IMPLEMENTED
        if (lowerer.diagnostics().has_errors()) {
            lowerer.diagnostics().report_all(src);
            has_errors = true;
        }
        */
        if (has_errors) {
            return 1;
        }
        
        // Only emit if no errors
        Emitter emitter;
        emitter.render(std::cout, low_doc);
    } catch (const std::exception& e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
