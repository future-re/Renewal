#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace renewal {

std::expected<std::string, std::string> generate_cmake(
    const std::filesystem::path& root_path);

}
