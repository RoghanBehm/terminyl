#include "error.hpp"
#include <algorithm>

void Diagnostic::report(const SourceFile& source, std::ostream& out) const {

    std::string line = source.get_line(span_.start.line);
    
    out << source.filename << ":" 
        << span_.start.line << ":" 
        << span_.start.column << ": ";
    
    // Level
    if (level_ == ErrorLevel::Error) {
        out << "error: ";
    } else if (level_ == ErrorLevel::Warning) {
        out << "warning: ";
    } else {
        out << "fatal error: ";
    }
    
    out << message_ << "\n";
    out << line << "\n";
    
    out << std::string(span_.start.column - 1, ' ') << "^\n";
}


void DiagnosticSet::report_all(const SourceFile& source, std::ostream& out) const {
    for (const auto& diag : diagnostics_) {
        diag.report(source, out);
    }
    
    // Summary
    size_t errors = std::count_if(diagnostics_.begin(), diagnostics_.end(),
        [](const Diagnostic& d) { return d.level() == ErrorLevel::Error; });
    
    if (errors > 0) {
        out << "\nFound " << errors << " error" << (errors != 1 ? "s" : "") << "\n";
    }
}