#include "document.hpp"
#include <memory>

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

Document::Expr::Ptr Document::Expr::make_logical(Document::Expr::Ptr lhs,
                                                 Token op,
                                                 Document::Expr::Ptr rhs,
                                                 SourceSpan sp) {
  return std::make_shared<Document::Expr>(
      Document::Expr::Logical{std::move(lhs), op, std::move(rhs)}, sp);
}

Document::Expr::Ptr Document::Expr::make_binary(Document::Expr::Ptr lhs,
                                                Token op,
                                                Document::Expr::Ptr rhs,
                                                SourceSpan sp) {
  return std::make_shared<Document::Expr>(
      Document::Expr::Binary{std::move(lhs), op, std::move(rhs)}, sp);
}

Document::Expr::Ptr
Document::Expr::make_unary(Token op, Document::Expr::Ptr rhs, SourceSpan sp) {
  return std::make_shared<Document::Expr>(
      Document::Expr::Unary{op, std::move(rhs)}, sp);
}

Document::Expr::Ptr Document::Expr::make_num(double v, SourceSpan sp) {
  return std::make_shared<Document::Expr>(Document::Expr::Num{v}, sp);
}
Document::Expr::Ptr Document::Expr::make_str(std::string s, SourceSpan sp) {
  return std::make_shared<Document::Expr>(Document::Expr::Str{std::move(s)},
                                          sp);
}

Document::Expr::Ptr
Document::Expr::make_call(std::string c, std::vector<Ptr> args, SourceSpan sp) {
  return std::make_shared<Document::Expr>(
      Document::Expr::Call{std::move(c), std::move(args)}, sp);
}

Document::Expr::Ptr Document::Expr::make_bool(bool b, SourceSpan sp) {
  return std::make_shared<Document::Expr>(Document::Expr::Bool{b}, sp);
}

Document::Expr::Ptr Document::Expr::make_var(std::string name, SourceSpan sp) {
  return std::make_shared<Expr>(Var{std::move(name)}, sp);
}

Document::Inline::Ptr
Document::Inline::make_let(std::string name, Expr::Ptr value, SourceSpan sp) {
  return std::make_shared<Inline>(Let{std::move(name), std::move(value)}, sp);
}

Document::Expr::Ptr Document::Expr::make_fn(std::vector<std::string> params,
                                            Expr::Ptr body, SourceSpan sp) {
  return std::make_shared<Expr>(Fn{std::move(params), std::move(body)}, sp);
}

Document::Expr::Ptr Document::Expr::make_none(SourceSpan sp) {
  return std::make_shared<Expr>(None{}, sp);
}

Document::Block::Ptr Document::Block::make_heading(int level, std::string text,
                                                   SourceSpan sp) {
  return std::make_shared<Block>(Heading{level, std::move(text)}, sp);
}

Document::Block::Ptr
Document::Block::make_paragraph(std::vector<Inline::Ptr> inlines,
                               SourceSpan sp) {
  return std::make_shared<Block>(Paragraph{std::move(inlines)}, sp);
}

Document::Block::Ptr
Document::Block::make_while(std::shared_ptr<Expr> cond, std::vector<Block::Ptr> body, SourceSpan sp) {
  return std::make_shared<Block>(While{std::move(cond), std::move(body)}, sp);
}


Document::Block::Ptr
Document::Block::make_assign(std::string name, Expr::Ptr value, SourceSpan sp) {
  return std::make_shared<Block>(Assign{std::move(name), std::move(value)}, sp);
}

Document::Block::Ptr
Document::Block::make_exprstmt(Expr::Ptr expr, SourceSpan sp) {
  return std::make_shared<Block>(ExprStmt{std::move(expr)}, sp);
}

