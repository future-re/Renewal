#pragma once

#include "renewal/diagnostic.hpp"
#include "renewal/manifest.hpp"

#include <expected>
#include <filesystem>
#include <vector>

namespace renewal {

std::vector<Diagnostic> validate_package(
    const std::filesystem::path& package_root, const PackageManifest& manifest);

std::expected<void, std::string> validator(std::filesystem::path package_root);

}  // namespace renewal
