#include "lowerer.hpp"
#include <iostream>
#include <sstream>

std::string Lowerer::toString(const Value &val) {
  return std::visit(
      [](auto const &x) -> std::string {
        using T = std::remove_cvref_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double>) {
          std::ostringstream oss;
          oss << x;
          return oss.str();
        } else {
          return x;
        }
      },
      val.v);
}

Document Lowerer::lower() {
  Document out;

  for (auto const &blk : getDoc().blocks()) {
    std::visit(
        [&](auto const &b) {
          using T = std::remove_cvref_t<decltype(b)>;

          if constexpr (std::is_same_v<T, Document::Heading>) {
            out.add(b);
          } else if constexpr (std::is_same_v<T, Document::Paragraph>) {
            Document::Paragraph p = b;
            p.inlines = lowerInlines(b.inlines);
            out.add(std::move(p));
          }
        },
        blk);
  }

  return out;
}

std::vector<Document::InlinePtr>
Lowerer::lowerInlines(const std::vector<Document::InlinePtr> &inlines) {
  std::vector<Document::InlinePtr> out;
  out.reserve(inlines.size());

  for (auto const &inl : inlines) {
    std::visit(
        [&](auto const &node) {
          using T = std::remove_cvref_t<decltype(node)>;

          if constexpr (std::is_same_v<T, Document::Inline::Text>) {
            out.push_back(inl); // pass as is

          } else if constexpr (std::is_same_v<T, Document::Inline::Code>) {
            out.push_back(inl); // pass as is

          } else if constexpr (std::is_same_v<T, Document::Inline::Italic>) {
            auto kids = lowerInlines(node.children);
            out.push_back(
                Document::Inline::make_italic(std::move(kids), inl->span));

          } else if constexpr (std::is_same_v<T, Document::Inline::Bold>) {
            auto kids = lowerInlines(node.children);
            out.push_back(
                Document::Inline::make_bold(std::move(kids), inl->span));

          } else if constexpr (std::is_same_v<T, Document::Inline::Splice>) {
            auto value = eval(*node.expr);
            out.push_back(
                Document::Inline::make_text(toString(value), inl->span));
          }
        },
        inl->node);
  }

  return out;
}

Lowerer::Value Lowerer::eval(const Document::Expr &expr) {
  return std::visit(
      [&](auto const &node) -> Value {
        using T = std::remove_cvref_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Document::Expr::Num>) {
          return Value{node.value};

        } else if constexpr (std::is_same_v<T, Document::Expr::Str>) {
          return Value{node.value}; // assuming Value supports std::string

        } else if constexpr (std::is_same_v<T, Document::Expr::Binary>) {

          auto lhs = eval(*node.lhs);
          auto rhs = eval(*node.rhs);
          Value value{};

          if (node.op.getType() != TokenType::PLUS) {
            throw std::runtime_error("unsupported operator");
          }
          if (auto a = std::get_if<double>(&lhs.v); a) {
            if (auto b = std::get_if<double>(&rhs.v); b) {
              return Value{*a + *b};
            }
          }
          return Value{toString(lhs) + toString(rhs)};

        } else {
          std::cerr << "Unhandled Expr variant alternative\n";
          return Value{};
        }
      },
      expr.node);
}
