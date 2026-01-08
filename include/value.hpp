#pragma once
#include "document.hpp"
#include "environment.hpp"
#include <memory>
#include <string>
#include <variant>

struct BuiltinFunction {
  std::string name;
  int arity;
};

struct UserFunction {
  std::vector<std::string> params;
  Document::ExprPtr body;
  std::shared_ptr<Environment> closure;
};
struct Error {
  std::string message;
};

struct Value {
  std::variant<double, std::string, bool, Error, UserFunction, BuiltinFunction, std::monostate> v;
};