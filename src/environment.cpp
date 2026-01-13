#include "environment.hpp"
#include "value.hpp"

Environment::Environment(std::shared_ptr<Environment> enc)
    : enclosing(std::move(enc)) {}

void Environment::define(const std::string &name, Value val) {
  values[name] = std::move(val);
}

bool Environment::assign(const std::string& name, const Value& v) {
  if (values.contains(name)) { values[name] = v; return true; }
  if (enclosing) return enclosing->assign(name, v);
  return false;
}


std::optional<Value> Environment::get(const std::string &name) const {
  if (auto it = values.find(name); it != values.end()) {
    return it->second;
  }
  if (enclosing)
    return enclosing->get(name);
  return std::nullopt;
}

std::shared_ptr<Environment> Environment::createGlobal() {
  auto env = std::make_shared<Environment>();

  env->define("max", Value{BuiltinFunction{"max", 2}});
  env->define("min", Value{BuiltinFunction{"min", 2}});
  env->define("len", Value{BuiltinFunction{"len", 1}});
  env->define("abs", Value{BuiltinFunction{"abs", 1}});

  env->define("push", Value{BuiltinFunction{"push", -1}});
  env->define("pop", Value{BuiltinFunction{"pop", 1}});

  return env;
}