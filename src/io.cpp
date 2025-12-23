#include "io.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void write_file(const std::string& path, std::string_view contents) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file for write: " + path);
    f.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!f) throw std::runtime_error("Failed to write file: " + path);
}
