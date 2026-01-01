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
          return evalBinaryOp(lhs, rhs, node.op.getType());
        } else {
          return Value{Error{"Lowerer::eval(): Unhandled expr alternative"}};
        }
      },
      expr.node);
}

template<typename T>
std::optional<Lowerer::Value> Lowerer::tryBinaryOp(const Value& lhs, const Value& rhs, TokenType op) {
    auto l = std::get_if<T>(&lhs.v);
    auto r = std::get_if<T>(&rhs.v);
    if (!l || !r) return std::nullopt;
    
    if constexpr (std::is_same_v<T, std::string>) {
        // Only support concatenation for strings
        if (op == TokenType::PLUS) return Value{*l + *r};
        return std::nullopt;
    } else if constexpr (std::is_same_v<T, bool>) {
        // Only support comparisons for bools
        switch (op) {
            case TokenType::EQUAL_EQUAL: return Value{*l == *r};
            case TokenType::BANG_EQUAL: return Value{*l != *r};
            default: return std::nullopt;
        }
    } else {
        switch (op) {
            case TokenType::PLUS: return Value{*l + *r};
            case TokenType::MINUS: return Value{*l - *r};
            case TokenType::STAR: return Value{*l * *r};
            case TokenType::SLASH: return Value{*l / *r};
            case TokenType::EQUAL_EQUAL: return Value{*l == *r};
            case TokenType::BANG_EQUAL: return Value{*l != *r};
            case TokenType::GREATER: return Value{*l > *r};
            case TokenType::GREATER_EQUAL: return Value{*l >= *r};
            case TokenType::LESS: return Value{*l < *r};
            case TokenType::LESS_EQUAL: return Value{*l <= *r};
            default: return std::nullopt;
        }
    }
}

Lowerer::Value Lowerer::evalBinaryOp(const Value& lhs, const Value& rhs, TokenType op) {
    if (auto result = tryBinaryOp<double>(lhs, rhs, op)) return *result;
    
    // Mixed-type equality comparisons
    if (op == TokenType::EQUAL_EQUAL) return Value{false};
    if (op == TokenType::BANG_EQUAL) return Value{true};

    // String concatenation fallback for PLUS
    if (op == TokenType::PLUS) {
        return Value{toString(lhs) + toString(rhs)};
    }
    
    throw std::runtime_error("Type mismatch for operator");
}