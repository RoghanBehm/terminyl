#include "value.hpp"
Array::Array(std::vector<Value> elems) : elements(std::move(elems)) {}