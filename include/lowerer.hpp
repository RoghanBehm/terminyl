#include "document.hpp"
#include "error.hpp"
#include "value.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

class Lowerer {
public:
  explicit Lowerer(const Document &doc)
      : doc_(doc), environment_(Environment::createGlobal()) {
    initBuiltinHandlers();
  }
  const DiagnosticSet &diagnostics() const { return diagnostics_; }
  Document lower(); // lower doc_ into a new Document
  const Document &getDoc() const { return doc_; }

private:
  const Document &doc_;
  std::shared_ptr<Environment> environment_;
  std::vector<Document::InlinePtr>
  lowerInlines(const std::vector<Document::InlinePtr> &inlines);
  void error(std::string message, SourceSpan span);
  DiagnosticSet diagnostics_;
  bool isLetOnlyParagraph(const Document::Block::Paragraph &p);

  Value eval(const Document::Expr &expr);
  Value evalCall(const Document::Expr::Call &node, SourceSpan span);
  Value callFunction(const BuiltinFunction &func,
                     const std::vector<Value> &args, SourceSpan span);
  Value callFunction(const UserFunction &func, const std::vector<Value> &args,
                     SourceSpan span);

  template <typename T>
  std::optional<Value> tryBinaryOp(const Value &lhs, const Value &rhs,
                                   TokenType op);

  template <typename T>
  std::optional<Value> tryUnaryOp(const Value &rhs, TokenType op);

  Value evalLogicalOp(const Document::Expr::Logical &node, SourceSpan span);
  Value evalBinaryOp(const Value &lhs, const Value &rhs, TokenType op,
                     SourceSpan span);
  Value evalUnaryOp(const Value &rhs, TokenType op, SourceSpan span);
  std::string toString(const Value &v);
  void execStmt(const Document::BlockPtr &blk, Document &out);
  void execWhile(const Document::Block::While &w, SourceSpan span,
                 Document &out);

    bool isWhitespaceOnly(const std::vector<Document::InlinePtr> &inlines);
      
  using BuiltinHandler =
      std::function<Value(const std::vector<Value> &, SourceSpan)>;
  std::unordered_map<std::string, BuiltinHandler> builtin_handlers_;
  void initBuiltinHandlers();

  // Builtin implementations
  Value builtinMax(const std::vector<Value> &args, SourceSpan span);
  Value builtinMin(const std::vector<Value> &args, SourceSpan span);
  Value builtinLen(const std::vector<Value> &args, SourceSpan span);
  Value builtinAbs(const std::vector<Value> &args, SourceSpan span);
  Value builtinPush(const std::vector<Value> &args, SourceSpan span);
  Value builtinPop(const std::vector<Value> &args, SourceSpan span);

  // Type checking
  std::optional<double> asNumber(const Value &v, const std::string &context,
                                 SourceSpan span);
  std::optional<Array*> asArray(Value &v, const std::string &context,
                                 SourceSpan span);
  std::optional<const Array*>
  asArray(const Value &v, const std::string &context, SourceSpan span) const;
};