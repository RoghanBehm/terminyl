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

struct Value;
struct Array {
  std::vector<Value> elements;
  Array() = default;
  Array(std::vector<Value> elems);
};

struct Value {
  std::variant<double, std::string, bool, Error, UserFunction, BuiltinFunction, Array, std::monostate> v;
};