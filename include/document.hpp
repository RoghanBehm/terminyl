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

    struct Emph {
      std::vector<Ptr> children;
    };

    std::variant<Text, Emph> node;
    SourceSpan span{};

    Inline(Text t, SourceSpan sp) : node(std::move(t)), span(sp) {}
    Inline(Emph e, SourceSpan sp) : node(std::move(e)), span(sp) {}

    static Ptr make_text(std::string s, SourceSpan sp);
    static Ptr make_emph(std::vector<Ptr> children, SourceSpan sp);
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
