#pragma once
#include <memory>
#include <unordered_map>
#include <optional>
#include <string>


struct Value;

class Environment {
public:
  std::shared_ptr<Environment> enclosing;
  std::unordered_map<std::string, Value> values;
  
  explicit Environment(std::shared_ptr<Environment> enc = nullptr);
  
  void define(const std::string& name, Value val);
  
  std::optional<Value> get(const std::string& name) const;

  static std::shared_ptr<Environment> createGlobal();
};