#include "document.hpp"
#include "error.hpp"
#include <optional>

class Lowerer {
public:
  explicit Lowerer(const Document &doc) : doc_(doc) {}
  const DiagnosticSet &diagnostics() const { return diagnostics_; }
  Document lower(); // lower doc_ into a new Document
  const Document &getDoc() const { return doc_; }

private:
  const Document &doc_;

  std::vector<Document::InlinePtr>
  lowerInlines(const std::vector<Document::InlinePtr> &inlines);
  void error(std::string message, SourceSpan span);
  DiagnosticSet diagnostics_;
  struct Error {
    std::string message;
  };

  struct Value {
    std::variant<double, std::string, bool, Error> v;
  };

  Value eval(const Document::Expr &expr);

  template <typename T>
  std::optional<Value> tryBinaryOp(const Value &lhs, const Value &rhs,
                                   TokenType op);

  template <typename T>
  std::optional<Value> tryUnaryOp(const Value &rhs, TokenType op);

  Lowerer::Value evalLogicalOp(const Document::Expr::Logical &node,
                               SourceSpan span);
  Value evalBinaryOp(const Value &lhs, const Value &rhs, TokenType op,
                     SourceSpan span);
  Value evalUnaryOp(const Value &rhs, TokenType op, SourceSpan span);
  std::string toString(const Value &v);
};