#include "lowerer.hpp"
#include "token_type.hpp"
#include <optional>
#include <sstream>
#include <variant>

std::string Lowerer::toString(const Value &val) {
  return std::visit(
      [](auto const &x) -> std::string {
        using T = std::remove_cvref_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double> || std::is_same_v<T, bool>) {
          std::ostringstream oss;
          oss << std::boolalpha << x;
          return oss.str();
        } else if constexpr (std::is_same_v<T, Error>) {
          return "Error in Lowerer::toString";
        } else if constexpr (std::is_same_v<T, Function>) {
          return "Function";
        } else if constexpr (std::is_same_v<T, std::monostate>) {
          return "none";
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
            auto lowered = lowerInlines(b.inlines);

            if (lowered.empty() && !b.inlines.empty() &&
                isLetOnlyParagraph(b)) {
              return;
            }

            // Normal paragraph
            Document::Paragraph p = b;
            p.inlines = std::move(lowered);
            out.add(std::move(p));
          }
        },
        blk);
  }

  return out;
}

bool Lowerer::isLetOnlyParagraph(const Document::Paragraph &p) {
  bool has_let = false;
  bool has_visible = false;

  for (auto const &inl : p.inlines) {
    std::visit(
        [&](auto const &node) {
          using U = std::remove_cvref_t<decltype(node)>;

          if constexpr (std::is_same_v<U, Document::Inline::Let>) {
            has_let = true;
          } else if constexpr (std::is_same_v<U, Document::Inline::Text>) {
            for (unsigned char c : node.text) {
              if (!std::isspace(c)) {
                has_visible = true;
                break;
              }
            }
          } else {
            has_visible = true;
          }
        },
        inl->node);
  }

  return has_let && !has_visible;
}

std::vector<Document::InlinePtr>
Lowerer::lowerInlines(const std::vector<Document::InlinePtr> &inlines) {
  std::vector<Document::InlinePtr> out;
  out.reserve(inlines.size());

  for (auto const &inl : inlines) {
    std::visit(
        [&](auto const &node) {
          using T = std::remove_cvref_t<decltype(node)>;

          if constexpr (std::is_same_v<T, Document::Inline::Text> ||
                        std::is_same_v<T, Document::Inline::Code>) {
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
          } else if constexpr (std::is_same_v<T, Document::Inline::Let>) {
            Value val = eval(*node.value);
            environment_->define(node.name, val);
          }
        },
        inl->node);
  }

  return out;
}

Value Lowerer::eval(const Document::Expr &expr) {
  return std::visit(
      [&](auto const &node) -> Value {
        using T = std::remove_cvref_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Document::Expr::Num> ||
                      std::is_same_v<T, Document::Expr::Str> ||
                      std::is_same_v<T, Document::Expr::Bool>) {
          return Value{node.value};
        } else if constexpr (std::is_same_v<T, Document::Expr::Binary>) {
          auto lhs = eval(*node.lhs);
          auto rhs = eval(*node.rhs);
          return evalBinaryOp(lhs, rhs, node.op.getType(), expr.span);
        } else if constexpr (std::is_same_v<T, Document::Expr::Unary>) {
          auto rhs = eval(*node.rhs);
          return evalUnaryOp(rhs, node.op.getType(), expr.span);
        } else if constexpr (std::is_same_v<T, Document::Expr::Logical>) {
          auto lhs = eval(*node.lhs);
          auto rhs = eval(*node.rhs);
          return evalLogicalOp(node, expr.span);
        } else if constexpr (std::is_same_v<T, Document::Expr::Var>) {
          auto result = environment_->get(node.name);
          if (!result) {
            error("Undefined variable '" + node.name + "'", expr.span);
            return Value{Error{"Undefined variable"}};
          }
          return *result;
        } else if constexpr (std::is_same_v<T, Document::Expr::None>) {
          return Value{std::monostate{}};
        } else {
          error("Unhandled expression type", expr.span);
          return Value{Error{"Unhandled expr alternative"}};
        }
      },
      expr.node);
}

Value Lowerer::evalLogicalOp(const Document::Expr::Logical &node,
                             SourceSpan span) {
  auto lhs_ = eval(*node.lhs);
  auto l = std::get_if<bool>(&lhs_.v);
  if (!l) {
    error("Logical operator requires boolean operands", span);
    return Value{Error{"Logical op on non-bool"}};
  }

  if (node.op.getType() == TokenType::OR) {
    if (*l)
      return Value{true}; // short-circuit
  } else if (node.op.getType() == TokenType::AND) {
    if (!*l)
      return Value{false}; // short-circuit
  } else {
    error("Unknown logical operator", span);
    return Value{Error{"Unknown logical op"}};
  }

  auto rhs_ = eval(*node.rhs);
  auto r = std::get_if<bool>(&rhs_.v);
  if (!r) {
    error("Logical operator requires boolean operands", span);
    return Value{Error{"Logical op on non-bool"}};
  }
  return Value{node.op.getType() == TokenType::OR ? (*l || *r) : (*l && *r)};
}

template <typename T>
std::optional<Value> Lowerer::tryBinaryOp(const Value &lhs, const Value &rhs,
                                          TokenType op) {
  auto l = std::get_if<T>(&lhs.v);
  auto r = std::get_if<T>(&rhs.v);
  if (!l || !r)
    return std::nullopt;

  if constexpr (std::is_same_v<T, std::string>) {
    // Only support concatenation for strings
    if (op == TokenType::PLUS)
      return Value{*l + *r};
    return std::nullopt;

    switch (op) {
    case TokenType::PLUS:
      return Value{*l + *r};
    case TokenType::EQUAL_EQUAL:
      return Value{*l == *r};
    case TokenType::BANG_EQUAL:
      return Value{*l != *r};
    default:
      return std::nullopt;
    }


  } else if constexpr (std::is_same_v<T, bool>) {
    // Only support comparisons for bools
    switch (op) {
    case TokenType::EQUAL_EQUAL:
      return Value{*l == *r};
    case TokenType::BANG_EQUAL:
      return Value{*l != *r};
    default:
      return std::nullopt;
    }
  } else {
    switch (op) {
    case TokenType::PLUS:
      return Value{*l + *r};
    case TokenType::MINUS:
      return Value{*l - *r};
    case TokenType::STAR:
      return Value{*l * *r};
    case TokenType::SLASH:
      return Value{*l / *r};
    case TokenType::EQUAL_EQUAL:
      return Value{*l == *r};
    case TokenType::BANG_EQUAL:
      return Value{*l != *r};
    case TokenType::GREATER:
      return Value{*l > *r};
    case TokenType::GREATER_EQUAL:
      return Value{*l >= *r};
    case TokenType::LESS:
      return Value{*l < *r};
    case TokenType::LESS_EQUAL:
      return Value{*l <= *r};
    default:
      return std::nullopt;
    }
  }
}

template <typename T>
std::optional<Value> Lowerer::tryUnaryOp(const Value &rhs, TokenType op) {
  auto r = std::get_if<T>(&rhs.v);
  if (!r)
    return std::nullopt;

  if constexpr (std::is_same_v<T, double>) {
    switch (op) {
    case TokenType::PLUS:
      return Value{+(*r)};
    case TokenType::MINUS:
      return Value{-(*r)};
    default:
      return std::nullopt;
    }
  } else if constexpr (std::is_same_v<T, bool>) {
    switch (op) {
    case TokenType::BANG:
      return Value{!(*r)};
    default:
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }
}

Value Lowerer::evalBinaryOp(const Value &lhs, const Value &rhs, TokenType op,
                            SourceSpan span) {
  if (auto result = tryBinaryOp<double>(lhs, rhs, op))
    return *result;

  // Mixed-type equality comparisons
  if (op == TokenType::EQUAL_EQUAL)
    return Value{false};
  if (op == TokenType::BANG_EQUAL)
    return Value{true};

  // String concatenation fallback for PLUS
  if (op == TokenType::PLUS) {
    return Value{toString(lhs) + toString(rhs)};
  }

  error("Type mismatch for operator", span);
  return Value{Error{"Type mismatch for binary operator"}};
}

Value Lowerer::evalUnaryOp(const Value &rhs, TokenType op, SourceSpan span) {
  if (auto result = tryUnaryOp<double>(rhs, op))
    return *result;
  if (auto result = tryUnaryOp<bool>(rhs, op))
    return *result;

  error("Type mismatch for unary operator", span);
  return Value{Error{"Type mismatch for unary operator"}};
}

void Lowerer::error(std::string message, SourceSpan span) {
  diagnostics_.add(Diagnostic(ErrorLevel::Error, std::move(message), span));
}
