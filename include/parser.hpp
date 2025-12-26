#include "document.hpp"
#include "text_accumulator.hpp"
#include "token.hpp"
class Parser {
public:
  Parser(std::vector<Token> tokens);
  Document parse();

private:
  const std::vector<Token> tokens_;
  void skipBlanks();
  Document::Heading heading();
  Document::Paragraph paragraph();
  int current = 0;
  bool check(TokenType type);
  bool match(TokenType type);
  const Token &peek() const;
  const Token &previous() const;
  bool isAtEnd();
  const Token &advance();
  bool handleNewlineInParagraph(TextAccumulator &text, bool &consumed_any);

  template <typename Inliner>
  bool handleDelimitedInline(Document::Paragraph &para, TextAccumulator &text,
                             bool &consumed_any, TokenType delimiter,
                             std::string_view literal_fallback,
                             Inliner make_inline) {
    const Token &openDelim = advance();
    consumed_any = true;

    std::string content;
    while (!isAtEnd() && !check(delimiter) && !check(TokenType::NEWLINE)) {
      content += std::string(advance().getLexeme());
      consumed_any = true;
    }

    if (check(delimiter)) {
      const Token &closeDelim = advance();
      consumed_any = true;

      SourceSpan span{openDelim.span().start, closeDelim.span().end};
      para.inlines.push_back(make_inline(std::move(content), span));
      return true;
    }

    // Unclosed, treat as literal
    text.append(literal_fallback, openDelim.span().start);
    text.append(content, openDelim.span().start);
    return false;
  }

  bool handleBold(Document::Paragraph &para, TextAccumulator &text,
                  bool &consumed_any);
  bool handleCode(Document::Paragraph &para, TextAccumulator &text,
                  bool &consumed_any);
  bool handleItalic(Document::Paragraph &para, TextAccumulator &text,
                    bool &consumed_any);
  Document::Block block();
};
