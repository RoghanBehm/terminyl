#include "document.hpp"
#include <optional>

class Lowerer {
public:
  explicit Lowerer(const Document &doc) : doc_(doc) {}

  Document lower(); // lower doc_ into a new Document
  const Document& getDoc() const { return doc_; }

private:
  const Document& doc_;

  std::vector<Document::InlinePtr>
  lowerInlines(const std::vector<Document::InlinePtr>& inlines);

  struct Error {
    std::string message;
  };

  struct Value {
    std::variant<double, std::string, bool, Error> v;
  };

  Value eval(const Document::Expr& expr);

  template<typename T>
  std::optional<Value> tryBinaryOp(const Value& lhs, const Value& rhs, TokenType op);

  Value evalBinaryOp(const Value& lhs, const Value& rhs, TokenType op);
  std::string toString(const Value& v);
};