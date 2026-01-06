#pragma once
#include "document.hpp"
#include "environment.hpp"
#include <memory>
#include <string>
#include <variant>

struct Function {
  std::vector<std::string> params;
  Document::Expr::Ptr body;
  std::shared_ptr<Environment> closure;
};

struct Error {
  std::string message;
};

struct Value {
  std::variant<double, std::string, bool, Error, Function> v;
};