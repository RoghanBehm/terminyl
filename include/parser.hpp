#include "document.hpp"
#include "error.hpp"
#include "token.hpp"
#include <functional>
#include <initializer_list>
class Parser {
public:
  Parser(std::vector<Token> tokens);
  Document parse();

  const DiagnosticSet &diagnostics() const { return diagnostics_; }

private:
  const std::vector<Token> tokens_;
  void skipBlanks();
  Document::Heading heading();
  Document::Paragraph paragraph();
  int current = 0;
  bool check(TokenType type);
  bool match(TokenType type);
  bool match(std::initializer_list<TokenType> types);
  Document::Expr::Ptr
  laparse(const std::function<Document::Expr::Ptr()> &op_type,
          std::initializer_list<TokenType> types);
  const Token &peek() const;
  const Token &previous() const;
  bool isAtEnd();
  const Token &advance();
  Token consume(TokenType type, std::string message);
  std::vector<Document::InlinePtr>
  parseInlines(TokenType endToken = TokenType::NEWLINE);
  Document::InlinePtr parseBold();
  Document::InlinePtr parseItalic();
  Document::InlinePtr parseCode();
  Document::InlinePtr parseSplice();
  Document::Expr::Ptr equality();
  Document::Expr::Ptr comparison();
  Document::Expr::Ptr term();
  Document::Expr::Ptr factor();
  Document::Expr::Ptr unary();
  Document::Expr::Ptr primary();
  void synchronize();
  Document::Block block();
  DiagnosticSet diagnostics_;
  void error(std::string message, SourceSpan span);

};
