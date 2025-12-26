#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "source.hpp"

class Document {

public:
  struct Inline {
    using Ptr = std::shared_ptr<Inline>;

    struct Text {
      std::string text;
    };

    struct Bold {
      std::vector<Ptr> children;
    };

    struct Italic {
      std::vector<Ptr> children;
    };

    struct Code {
      std::string text;
    };


    std::variant<Text, Bold, Italic, Code> node;
    SourceSpan span{};

    Inline(Text t, SourceSpan sp) : node(std::move(t)), span(sp) {}
    Inline(Bold e, SourceSpan sp) : node(std::move(e)), span(sp) {}
    Inline(Italic i, SourceSpan sp) : node(std::move(i)), span(sp) {}
    Inline(Code c, SourceSpan sp) : node(std::move(c)), span(sp) {}

    static Ptr make_text(std::string s, SourceSpan sp);
    static Ptr make_bold(std::vector<Ptr> children, SourceSpan sp);
    static Ptr make_italic(std::vector<Ptr> children, SourceSpan sp);
    static Ptr make_code(std::string s, SourceSpan sp);
  };



  using InlinePtr = Inline::Ptr;
  struct Heading {
    int level = 0;
    SourceSpan span{};
    std::string text;
  };

  struct Paragraph {
    SourceSpan span{};
    std::vector<InlinePtr> inlines;
  };

  using Block = std::variant<Heading, Paragraph>;

  const std::vector<Block>& blocks() const { return blocks_; }
  void add(Block b) { blocks_.push_back(std::move(b)); }
  static Document parse(std::istream &in);

private:
  std::vector<Block> blocks_;
};
