#include "error.hpp"
#include <algorithm>
#include <iomanip>

std::string Diagnostic::get_level_name() const {
    switch (level_) {
        case ErrorLevel::Error:
            return "error";
        case ErrorLevel::Fatal:
            return "fatal error";
        case ErrorLevel::Warning:
            return "warning";
    }
    return "error";
}

const char* Diagnostic::get_level_colour() const {
    switch (level_) {
        case ErrorLevel::Error:
        case ErrorLevel::Fatal:
            return Colour::RED;
        case ErrorLevel::Warning:
            return Colour::YELLOW;
    }
    return "";
}

void Diagnostic::print_snippet(const SourceFile& source, std::ostream& out) const {

    size_t max_line = span_.end.line;
    auto line_num_width = std::to_string(max_line).length();
    
    size_t start_line = span_.start.line > 1 ? span_.start.line - 1 : span_.start.line;
    size_t end_line = std::min(static_cast<size_t>(span_.end.line + 1), source.get_line_count());
    
    // Separator
    std::string gutter_pad(line_num_width, ' ');
    out << Colour::BLUE << gutter_pad << " -->" << Colour::RESET
        << " " << source.filename << ":" << span_.start.line << ":" << span_.start.column << "\n";
    
    // 
    out << Colour::BLUE << gutter_pad << " |" << Colour::RESET << "\n";

    // Convert tabs to spaces
    auto expand_tabs = [](const std::string& line, size_t tab_width = 4) -> std::string {
        std::string result;
        for (char c : line) {
            if (c == '\t') {
                size_t spaces = tab_width - (result.length() % tab_width);
                result.append(spaces, ' ');
            } else {
                result += c;
            }
        }
        return result;
    };
    
    // Handles positioning in light of tabs
    auto visual_column = [](const std::string& line, size_t col, size_t tab_width = 4) -> size_t {
        size_t visual_pos = 0;
        for (size_t i = 0; i < col && i < line.length(); ++i) {
            if (line[i] == '\t') {
                visual_pos = ((visual_pos / tab_width) + 1) * tab_width;
            } else {
                visual_pos++;
            }
        }
        return visual_pos;
    };
    
    // Print code lines with context
    for (size_t line_num = start_line; line_num <= end_line; ++line_num) {
        std::string line = source.get_line(line_num);
        std::string display_line = expand_tabs(line);
        
        // Print line number and gutter
        out << Colour::BLUE 
            << std::setw(static_cast<int>(line_num_width)) 
            << line_num << " | " 
            << Colour::RESET;
            
        out << display_line << "\n";
        
        // Print underline only for error line
        if (line_num == span_.start.line) {
            std::string underline(line_num_width, ' ');
            out << Colour::BLUE << underline << " | " << Colour::RESET;
            
            // positioning
            size_t underline_pos = visual_column(line, span_.start.column - 1);
            out << std::string(underline_pos, ' ');
            
            // Underline
            const char* colour = get_level_colour();
            out << colour << "^ " << message_ << Colour::RESET << "\n";
        }
    }
    
    // Print closing gutter line
    out << Colour::BLUE << gutter_pad << " |" << Colour::RESET << "\n";
}

void Diagnostic::report(const SourceFile& source, std::ostream& out) const {

    const char* colour = get_level_colour();
    out << colour << Colour::BOLD << get_level_name() << Colour::RESET << ": ";
    out << Colour::BOLD << message_ << Colour::RESET << "\n";

    print_snippet(source, out);
}

void DiagnosticSet::report_all(const SourceFile& source, std::ostream& out) const {
    for (size_t i = 0; i < diagnostics_.size(); ++i) {
        diagnostics_[i].report(source, out);
        
        // Spacing between diagnostics
        if (i < diagnostics_.size() - 1) {
            out << "\n";
        }
    }
    
    // Summary
    size_t error_count = std::count_if(diagnostics_.begin(), diagnostics_.end(),
        [](const Diagnostic& d) { 
            return d.level() == ErrorLevel::Error || d.level() == ErrorLevel::Fatal; 
        });
    
    size_t warning_count = std::count_if(diagnostics_.begin(), diagnostics_.end(),
        [](const Diagnostic& d) { return d.level() == ErrorLevel::Warning; });
    
    if (error_count > 0 || warning_count > 0) {
        out << "\n" << Colour::BOLD;
        
        bool printed = false;
        if (error_count > 0) {
            out << Colour::RED << "error: could not compile due to " << error_count 
                << " error" << (error_count != 1 ? "s" : "");
            printed = true;
        }
        
        if (warning_count > 0) {
            if (printed) out << "; ";
            out << Colour::YELLOW << warning_count << " warning" 
                << (warning_count != 1 ? "s" : "") << " emitted";
        }
        
        out << Colour::RESET << "\n";
    }
}