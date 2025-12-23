#pragma once
#include <string>
#include <string_view>

std::string read_file(const std::string& path);
void write_file(const std::string& path, std::string_view contents);
