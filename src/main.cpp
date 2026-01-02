#include "emitter.hpp"
#include "lowerer.hpp"
#include "io.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "error.hpp"
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>

std::size_t get_terminal_width() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80; // fallback
}
#else
#include <sys/ioctl.h>
#include <unistd.h>

std::size_t get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return 80; // fallback
}
#endif

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
            std::cerr << "=== Lexer Errors ===\n";
            lexer.diagnostics().report_all(src);
            has_errors = true;
        }

        if (parser.diagnostics().has_errors()) {
            std::cerr << "\n=== Parser Errors ===\n";
            parser.diagnostics().report_all(src);
            has_errors = true;
        }

        if (lowerer.diagnostics().has_errors()) {
            std::cerr << "\n=== Evaluation Errors ===\n";
            lowerer.diagnostics().report_all(src);
            has_errors = true;
        }
        
        if (has_errors) {
            return 1;
        }
        
        // Only emit if no errors
        Style style;
        style.width = std::min(get_terminal_width() - 4, 120ul);
        Emitter emitter(style);
        emitter.render(std::cout, low_doc);
    } catch (const std::exception& e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
