#include "renewal/utils.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace renewal {
namespace utils {

namespace fs = std::filesystem;

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to read: " + path.string());
  }
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

void write_file(const fs::path& path, const std::string& content) {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("Failed to write: " + path.string());
  }
  output << content;
  if (!output) {
    throw std::runtime_error("Failed to write: " + path.string());
  }
}

std::string replace_all(std::string str, const std::string& from,
                        const std::string& to) {
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::string::npos) {
    str.replace(pos, from.length(), to);
    pos += to.length();
  }
  return str;
}

std::string trim(const std::string& str) {
  std::size_t begin = 0;
  while (begin < str.size() &&
         std::isspace(static_cast<unsigned char>(str[begin]))) {
    ++begin;
  }

  std::size_t end = str.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(str[end - 1]))) {
    --end;
  }

  return str.substr(begin, end - begin);
}

std::string strip_comment(const std::string& line) {
  bool in_string = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
      in_string = !in_string;
      continue;
    }
    if (!in_string && line[i] == '#') {
      return line.substr(0, i);
    }
  }
  return line;
}

}  // namespace utils
}  // namespace renewal
