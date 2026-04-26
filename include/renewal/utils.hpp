#pragma once
#include <filesystem>
#include <string>

namespace renewal {
namespace utils {

namespace fs = std::filesystem;
std::string read_file(const fs::path& path);
void write_file(const fs::path& path, const std::string& content);
std::string replace_all(std::string str, const std::string& from,
                        const std::string& to);
std::string trim(const std::string& str);
std::string strip_comment(const std::string& line);
}  // namespace utils
}  // namespace renewal
