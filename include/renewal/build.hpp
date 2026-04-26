#pragma once

#include "renewal/generator.hpp"
#include "renewal/toolchain.hpp"

#include <expected>
#include <filesystem>
#include <string>

namespace renewal {

struct BuildResult {
  GeneratedLayout layout;
  ToolchainInfo toolchain;
};

std::expected<BuildResult, std::string> build_project(
    const std::filesystem::path& root_path);

}  // namespace renewal
