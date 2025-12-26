#include "token.hpp"
#include "document.hpp"
#include "text_accumulator.hpp"
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
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd();
    const Token& advance();
    bool handleNewlineInParagraph(TextAccumulator& text, bool& consumed_any);
    bool handleBold(Document::Paragraph& para, TextAccumulator& text, bool& consumed_any);
    Document::Block block();
};