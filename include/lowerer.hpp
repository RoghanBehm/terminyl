#include "document.hpp"

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
  std::string toString(const Value& v);
};