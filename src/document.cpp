#include "document.hpp"

Document::Inline::Ptr Document::Inline::make_text(std::string s,
                                                  SourceSpan sp) {
  return std::make_shared<Inline>(Text{std::move(s)}, sp);
}

Document::Inline::Ptr Document::Inline::make_bold(std::vector<Ptr> children,
                                                  SourceSpan sp) {
  return std::make_shared<Inline>(Bold{std::move(children)}, sp);
}

Document::Inline::Ptr Document::Inline::make_code(std::string s,
                                                  SourceSpan sp) {
  return std::make_shared<Inline>(Code{std::move(s)}, sp);
}

Document::Inline::Ptr Document::Inline::make_italic(std::vector<Ptr> children,
                                                    SourceSpan sp) {
  return std::make_shared<Inline>(Italic{std::move(children)}, sp);
}

Document::Inline::Ptr Document::Inline::make_splice(std::shared_ptr<Expr> e,
                                                    SourceSpan sp) {
  return std::make_shared<Inline>(Splice{std::move(e)}, sp);
}

Document::Expr::Ptr Document::Expr::make_binary(Document::Expr::Ptr lhs,
                                                Token op,
                                                Document::Expr::Ptr rhs,
                                                SourceSpan sp) {
  return std::make_shared<Document::Expr>(
      Document::Expr::Binary{std::move(lhs), std::move(op), std::move(rhs)},
      sp);
}

Document::Expr::Ptr Document::Expr::make_num(double v, SourceSpan sp) {
  return std::make_shared<Document::Expr>(Document::Expr::Num{v}, sp);
}
Document::Expr::Ptr Document::Expr::make_str(std::string s, SourceSpan sp) {
  return std::make_shared<Document::Expr>(Document::Expr::Str{std::move(s)}, sp);
}
