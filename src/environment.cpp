#include "environment.hpp"
#include "value.hpp"

Environment::Environment(std::shared_ptr<Environment> enc) 
  : enclosing(std::move(enc)) {}

void Environment::define(const std::string& name, Value val) {
  values[name] = std::move(val);
}

std::optional<Value> Environment::get(const std::string& name) const {
  if (auto it = values.find(name); it != values.end()) {
    return it->second;
  }
  if (enclosing) return enclosing->get(name);
  return std::nullopt;
}