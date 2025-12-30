#include "lowerer.hpp"
#include "token_type.hpp"
#include <iostream>
#include <optional>
#include <sstream>

std::string Lowerer::toString(const Value &val) {
  return std::visit(
      [](auto const &x) -> std::string {
        using T = std::remove_cvref_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double> || std::is_same_v<T, bool>) {
          std::ostringstream oss;
          oss << x;
          return oss.str();
        } else if constexpr (std::is_same_v<T, Error>) {
          return "Error in Lowerer::toString";
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
          return Value{node.value};

        } else if constexpr (std::is_same_v<T, Document::Expr::Binary>) {
          auto lhs = eval(*node.lhs);
          auto rhs = eval(*node.rhs);

          auto both_double = [&]() -> std::optional<std::pair<double, double>> {
            if (auto l = std::get_if<double>(&lhs.v))
              if (auto r = std::get_if<double>(&rhs.v))
                return {{*l, *r}};
            return std::nullopt;
          };

          const auto op_type = node.op.getType();

          switch (op_type) {
          case TokenType::PLUS: {
            if (auto lr = both_double())
              return Value{lr->first + lr->second};
            return Value{toString(lhs) + toString(rhs)};
          }

          case TokenType::MINUS: {
            if (auto lr = both_double())
              return Value{lr->first - lr->second};
            throw std::runtime_error("'-' expects two numbers");
          }

          case TokenType::SLASH: {
            if (auto lr = both_double())
              return Value{lr->first / lr->second};
            throw std::runtime_error("'/' expects two numbers");
          }

          case TokenType::STAR: {
            if (auto lr = both_double())
              return Value{lr->first * lr->second};
            throw std::runtime_error("'*' expects two numbers");
          }

          case TokenType::BANG_EQUAL: {
            if (auto lr = both_double())
              return Value{lr->first != lr->second};
            throw std::runtime_error("'!=' expects comparable types");
          }

          case TokenType::EQUAL_EQUAL: {
            if (auto lr = both_double())
              return Value{lr->first == lr->second};
            throw std::runtime_error("'==' expects comparable types");
          }

          case TokenType::GREATER: {
            if (auto lr = both_double())
              return Value{lr->first > lr->second};
            throw std::runtime_error("'>' expects comparable types");
          }

          case TokenType::GREATER_EQUAL: {
            if (auto lr = both_double())
              return Value{lr->first >= lr->second};
            throw std::runtime_error("'>=' expects comparable types");
          }

          case TokenType::LESS: {
            if (auto lr = both_double())
              return Value{lr->first < lr->second};
            throw std::runtime_error("'<' expects comparable types");
          }

          case TokenType::LESS_EQUAL: {
            if (auto lr = both_double())
              return Value{lr->first <= lr->second};
            throw std::runtime_error("'<=' expects comparable types");
          }

          default:
            return Value{Error{"Lowerer::eval(): Unhandled binary operator"}};
          }

        } else {
          return Value{Error{"Lowerer::eval(): Unhandled expr alternative"}};
        }
      },
      expr.node);
}
