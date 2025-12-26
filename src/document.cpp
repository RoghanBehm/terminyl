#include "document.hpp"

Document::Inline::Ptr Document::Inline::make_text(std::string s, SourceSpan sp) {
    return std::make_shared<Inline>(Text{std::move(s)}, sp);
}

Document::Inline::Ptr Document::Inline::make_emph(std::vector<Ptr> children, SourceSpan sp) {
    return std::make_shared<Inline>(Emph{std::move(children)}, sp);
}
