#pragma once

#include "renewal/generator.hpp"

#include <expected>
#include <filesystem>
#include <string>

namespace renewal {

std::expected<GeneratedLayout, std::string> build_project(
    const std::filesystem::path& root_path);

}  // namespace renewal
