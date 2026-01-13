#include "lowerer.hpp"
#include "document.hpp"
#include "token_type.hpp"
#include "value.hpp"
#include <optional>
#include <sstream>
#include <variant>

static bool isWhitespaceOnly(const std::vector<Document::InlinePtr> &inlines);

std::string Lowerer::toString(const Value &val) {
  return std::visit(
      [this](auto const &x) -> std::string {
        using T = std::remove_cvref_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double> || std::is_same_v<T, bool>) {
          std::ostringstream oss;
          oss << std::boolalpha << x;
          return oss.str();
        } else if constexpr (std::is_same_v<T, Error>) {
          return "Error in Lowerer::toString";
        } else if constexpr (std::is_same_v<T, BuiltinFunction> ||
                             std::is_same_v<T, UserFunction>) {
          return "Function";
        } else if constexpr (std::is_same_v<T, std::monostate>) {
          return "none";
        } else if constexpr (std::is_same_v<T, Array>) {
          std::ostringstream oss;
          oss << "[";
          for (size_t i = 0; i < x.elements.size(); ++i) {
            if (i > 0)
              oss << ", ";
            oss << toString(x.elements[i]);
          }
          oss << "]";
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
    execStmt(blk, out);
  }
  return out;
}

void Lowerer::execStmt(const Document::BlockPtr &blk, Document &out) {
  std::visit(
      [&](auto const &b) {
        using T = std::remove_cvref_t<decltype(b)>;

        if constexpr (std::is_same_v<T, Document::Block::Assign>) {
          Value v = eval(*b.value);
          if (!environment_->assign(b.name, v)) {
            error("Assign to undefined variable '" + b.name + "'", blk->span);
          }
          return;

        } else if constexpr (std::is_same_v<T, Document::Block::ExprStmt>) {
          Value v = eval(*b.expr);
          // emit as paragraph (or inline text)
          std::vector<Document::InlinePtr> inls;
          inls.push_back(Document::Inline::make_text(toString(v), blk->span));
          out.add(Document::Block::make_paragraph(std::move(inls), blk->span));
        } else if constexpr (std::is_same_v<T, Document::Block::Paragraph>) {
          auto lowered = lowerInlines(b.inlines);

          // Don’t emit paragraphs that produce no visible output
          if (isWhitespaceOnly(lowered))
            return;

          out.add(
              Document::Block::make_paragraph(std::move(lowered), blk->span));
        } else if constexpr (std::is_same_v<T, Document::Block::Heading>) {
          out.add(Document::Block::make_heading(b.level, b.text, blk->span));
        } else if constexpr (std::is_same_v<T, Document::Block::While>) {
          execWhile(b, blk->span, out);
        }
      },
      blk->node);
}

void Lowerer::execWhile(const Document::Block::While &w, SourceSpan span,
                        Document &out) {
  std::vector<Document::BlockPtr> loop_blocks;

  while (true) {
    Value condv = eval(*w.cond);
    auto b = std::get_if<bool>(&condv.v);
    if (!b) {
      error("While condition must be boolean", span);
      return;
    }
    if (!*b)
      break;

    // Collect blocks from this iteration into a temp document
    Document temp_doc;
    for (auto const &stmt : w.body) {
      execStmt(stmt, temp_doc);
    }

    // Accumulate blocks found in loop
    for (auto &blk : temp_doc.blocks()) {
      loop_blocks.push_back(blk);
    }
  }

  // Wrap iterations in group
  if (!loop_blocks.empty()) {
    out.add(Document::Block::make_group(std::move(loop_blocks), span));
  }
}

bool Lowerer::isLetOnlyParagraph(const Document::Block::Paragraph &p) {
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
        if constexpr (std::is_same_v<T, Document::Expr::Call>) {
          return evalCall(node, expr.span);
        } else if constexpr (std::is_same_v<T, Document::Expr::Num> ||
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
        } else if constexpr (std::is_same_v<T, Document::Expr::Fn>) {
          return Value{UserFunction{node.params, node.body, environment_}};
        } else if constexpr (std::is_same_v<T, Document::Expr::ArrayLiteral>) {
          std::vector<Value> elements;
          elements.reserve(node.elements.size());
          for (auto const &elem : node.elements) {
            elements.push_back(eval(*elem));
          }
          return Value{Array{std::move(elements)}};
        } else if constexpr (std::is_same_v<T, Document::Expr::Index>) {
          auto obj_value = eval(*node.object);
          auto *arr = std::get_if<Array>(&obj_value.v);

          if (!arr) {
            error("Cannot index non-array type", expr.span);
            return Value{Error{"Type error: not an array"}};
          }

          auto idx_value = eval(*node.index);
          auto *idx_num = std::get_if<double>(&idx_value.v);

          if (!idx_num) {
            error("Array index must be a number", expr.span);
            return Value{Error{"Type error: index not a number"}};
          }

          int idx = static_cast<int>(*idx_num);

          // Negative indices wrap
          if (idx < 0) {
            idx = static_cast<int>(arr->elements.size()) + idx;
          }

          if (idx < 0 || idx >= static_cast<int>(arr->elements.size())) {
            error("Array index out of bounds", expr.span);
            return Value{Error{"Index out of bounds"}};
          }

          return arr->elements[idx];
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

Value Lowerer::evalCall(const Document::Expr::Call &node, SourceSpan span) {

  auto func_value = environment_->get(node.callee);
  if (!func_value) {
    error("Undefined function '" + node.callee + "'", span);
    return Value{Error{"Undefined function"}};
  }

  // Eval args
  std::vector<Value> args;
  args.reserve(node.args.size());
  for (auto &arg : node.args) {
    args.push_back(eval(*arg));
  }

  return std::visit(
      [&](auto const &v) -> Value {
        using T = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::is_same_v<T, BuiltinFunction> ||
                      std::is_same_v<T, UserFunction>) {
          return callFunction(v, args, span);
        } else {
          error("'" + node.callee + "' is not a function", span);
          return Value{Error{"Not a function"}};
        }
      },
      func_value->v);
}

Value Lowerer::callFunction(const BuiltinFunction &func,
                            const std::vector<Value> &args, SourceSpan span) {
  // Check arity
  if (func.arity != -1) {
    if (args.size() != static_cast<size_t>(func.arity)) {
      error("Expected " + std::to_string(func.arity) + " arguments", span);
      return Value{Error{"Arity mismatch"}};
    }
  }

  if (func.name == "max") {
    auto a = std::get_if<double>(&args[0].v);
    auto b = std::get_if<double>(&args[1].v);
    if (!a || !b) {
      error("#max(x, y) requires numeric arguments", span);
      return Value{Error{"Type error"}};
    }
    return Value{std::max(*a, *b)};
  } else if (func.name == "min") {
    auto a = std::get_if<double>(&args[0].v);
    auto b = std::get_if<double>(&args[1].v);
    if (!a || !b) {
      error("#min(x, y) requires numeric arguments", span);
      return Value{Error{"Type error"}};
    }
    return Value{std::min(*a, *b)};
  } else if (func.name == "len") {
    return std::visit(
        [&](auto const &v) -> Value {
          using T = std::remove_cvref_t<decltype(v)>;
          if constexpr (std::is_same_v<T, std::string>) {
            return Value{static_cast<double>(v.size())};
          } else if constexpr (std::is_same_v<T, Array>) {
            return Value{static_cast<double>(v.elements.size())};
          } else {
            error("len() requires a string or array argument", span);
            return Value{Error{"Type error"}};
          }
        },
        args[0].v);
  } else if (func.name == "abs") {
    auto value = std::get_if<double>(&args[0].v);
    if (!value) {
      error("#abs(x) requires numeric argument", span);
      return Value{Error{"Type error"}};
    }
    return Value{std::abs(*value)};
  } else if (func.name == "push") {
    if (args.empty()) {
      error("push() requires at least 1 argument", span);
      return Value{Error{"Arity mismatch"}};
    }

    auto *arr = std::get_if<Array>(&args[0].v);
    if (!arr) {
      error("First argument to push() must be an array", span);
      return Value{Error{"Type error"}};
    }
    Array result = *arr;
    for (size_t i = 1; i < args.size(); ++i) {
      result.elements.push_back(args[i]);
    }

    return Value{std::move(result)};
  } else if (func.name == "pop") {
    auto *arr = std::get_if<Array>(&args[0].v);
    if (!arr) {
      error("pop() requires an array argument", span);
      return Value{Error{"Type error"}};
    }

    if (arr->elements.empty()) {
      error("Cannot pop from empty array", span);
      return Value{Error{"Runtime error"}};
    }

    return arr->elements.back();
  }
  error("Unknown built-in function: " + func.name, span);
  return Value{Error{"Unknown function"}};
}

Value Lowerer::callFunction(const UserFunction &func,
                            const std::vector<Value> &args, SourceSpan span) {
  // Check arity
  if (args.size() != func.params.size()) {
    error("Expected " + std::to_string(func.params.size()) + " arguments",
          span);
    return Value{Error{"Arity mismatch"}};
  }

  auto call_env = std::make_shared<Environment>(func.closure);

  for (size_t i = 0; i < func.params.size(); ++i) {
    call_env->define(func.params[i], args[i]);
  }

  auto previous_env = environment_;
  environment_ = call_env;
  Value result = eval(*func.body);
  environment_ = previous_env;

  return result;
}

static bool isWhitespaceOnly(const std::vector<Document::InlinePtr> &inlines) {
  for (auto const &inl : inlines) {
    bool has_visible = std::visit(
        [&](auto const &node) -> bool {
          using U = std::remove_cvref_t<decltype(node)>;

          if constexpr (std::is_same_v<U, Document::Inline::Text> ||
                        std::is_same_v<U, Document::Inline::Code>) {
            for (unsigned char c : node.text) {
              if (!std::isspace(c))
                return true;
            }
            return false;
          } else if constexpr (std::is_same_v<U, Document::Inline::Bold> ||
                               std::is_same_v<U, Document::Inline::Italic>) {
            return !isWhitespaceOnly(node.children);
          } else {
            return true; // Other node types are visible
          }
        },
        inl->node);

    if (has_visible)
      return false;
  }
  return true;
}
