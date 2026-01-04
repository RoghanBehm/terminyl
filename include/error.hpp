#pragma once
#include <cstddef>
#include <string>
#include <iostream>
#include <vector>
#include "source.hpp"

enum class ErrorLevel : std::uint8_t {
    Warning,
    Error,
    Fatal
};

namespace Colour {
    inline const char* const RED = "\033[1;31m";
    inline const char* const YELLOW = "\033[1;33m";
    inline const char* const BLUE = "\033[1;34m";
    inline const char* const RESET = "\033[0m";
    inline const char* const BOLD = "\033[1m";
}

struct SourceFile {
    std::string filename;
    std::string content;
    
    
    [[nodiscard]] std::string get_line(size_t line_num) const {
        size_t current_line = 1;
        size_t line_start = 0;
        
        for (size_t i = 0; i < content.length(); ++i) {
            if (content[i] == '\n') {
                if (current_line == line_num) {
                    return content.substr(line_start, i - line_start);
                }
                current_line++;
                line_start = i + 1;
            }
        }
        
        if (current_line == line_num) {
            return content.substr(line_start);
        }
        
        return "";
    }
    
    size_t get_line_count() const {
        size_t count = 1;
        for (char c : content) {
            if (c == '\n') count++;
        }
        return count;
    }
};

class Diagnostic {
public:
    Diagnostic(ErrorLevel level, std::string message, SourceSpan span) 
        : level_(level), message_(std::move(message)), span_(span) {}

    void report(const SourceFile& source, std::ostream& out) const;
    ErrorLevel level() const { return level_; }

private:
    ErrorLevel level_;
    std::string message_;
    SourceSpan span_;
    
    void print_snippet(const SourceFile& source, std::ostream& out) const;
    const char* get_level_colour() const;
    std::string get_level_name() const;
};

class DiagnosticSet {
public:
    void add(Diagnostic diag) {
        if (diag.level() == ErrorLevel::Error || diag.level() == ErrorLevel::Fatal) {
            has_errors_ = true;
        }
        diagnostics_.push_back(std::move(diag));
    }
    
    void report_all(const SourceFile& source, std::ostream& out = std::cerr) const;
    
    bool has_errors() const { return has_errors_; }
    bool empty() const { return diagnostics_.empty(); }
    size_t count() const { return diagnostics_.size(); }
    
private:
    std::vector<Diagnostic> diagnostics_;
    bool has_errors_ = false;
};