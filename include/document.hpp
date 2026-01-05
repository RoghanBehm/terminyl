#pragma once
#include "source.hpp"
#include "token.hpp"
#include <memory>
#include <string>
#include <variant>
#include <vector>

class Document {

public:
  struct Expr {

    using Ptr = std::shared_ptr<Expr>;

    struct Num {
      double value;
    };
    struct Var {
      std::string name;
    };
    struct Str {
      std::string value;
    };
    struct Call {
      std::string callee;
      std::vector<Ptr> args;
    };

    struct Logical {
      Ptr lhs;
      Token op;
      Ptr rhs;
    };
    struct Binary {
      Ptr lhs;
      Token op;
      Ptr rhs;
    };
    struct Unary {
      Token op;
      Ptr rhs;
    };
    struct Bool {
      bool value;
    };

    std::variant<Num, Var, Str, Call, Logical, Binary, Unary, Bool> node;
    SourceSpan span{};

    Expr(Num n, SourceSpan sp) : node(n), span(sp) {}
    Expr(Var v, SourceSpan sp) : node(std::move(v)), span(sp) {}
    Expr(Str s, SourceSpan sp) : node(std::move(s)), span(sp) {}
    Expr(Call c, SourceSpan sp) : node(std::move(c)), span(sp) {}
    Expr(Logical l, SourceSpan sp) : node(std::move(l)), span(sp) {}
    Expr(Binary b, SourceSpan sp) : node(std::move(b)), span(sp) {}
    Expr(Unary u, SourceSpan sp) : node(std::move(u)), span(sp) {}
    Expr(Bool b, SourceSpan sp) : node(b), span(sp) {}

    static Ptr make_num(double v, SourceSpan sp);
    static Ptr make_str(std::string s, SourceSpan sp);
    static Ptr make_logical(Ptr lhs, Token op, Ptr rhs, SourceSpan sp);
    static Ptr make_binary(Ptr lhs, Token op, Ptr rhs, SourceSpan sp);
    static Ptr make_unary(Token op, Ptr rhs, SourceSpan sp);
    static Ptr make_bool(bool b, SourceSpan sp);
  };
  struct Inline {
    using Ptr = std::shared_ptr<Inline>;

    struct Splice {
      Expr::Ptr expr;
    };

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

    std::variant<Text, Bold, Italic, Code, Splice> node;
    SourceSpan span{};

    Inline(Text t, SourceSpan sp) : node(std::move(t)), span(sp) {}
    Inline(Bold e, SourceSpan sp) : node(std::move(e)), span(sp) {}
    Inline(Italic i, SourceSpan sp) : node(std::move(i)), span(sp) {}
    Inline(Code c, SourceSpan sp) : node(std::move(c)), span(sp) {}
    Inline(Splice s, SourceSpan sp) : node(std::move(s)), span(sp) {}

    static Ptr make_text(std::string s, SourceSpan sp);
    static Ptr make_bold(std::vector<Ptr> children, SourceSpan sp);
    static Ptr make_italic(std::vector<Ptr> children, SourceSpan sp);
    static Ptr make_code(std::string s, SourceSpan sp);
    static Ptr make_splice(Expr::Ptr e, SourceSpan sp);
  };

  using InlinePtr = Inline::Ptr;
  using ExprPtr = Expr::Ptr;
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

  const std::vector<Block> &blocks() const { return blocks_; }
  void add(Block b) { blocks_.push_back(std::move(b)); }
  static Document parse(std::istream &in);

private:
  std::vector<Block> blocks_;
};
