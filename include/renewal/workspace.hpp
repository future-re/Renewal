#pragma once

#include "renewal/diagnostic.hpp"
#include "renewal/manifest.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace renewal {

struct PackageInfo {
  std::filesystem::path root;
  PackageManifest manifest;
};

struct WorkspaceInfo {
  std::filesystem::path requested_root;
  std::vector<PackageInfo> packages;
};

struct WorkspaceLoadResult {
  WorkspaceInfo workspace;
  std::vector<Diagnostic> diagnostics;

  [[nodiscard]] bool ok() const { return diagnostics.empty(); }
};

WorkspaceLoadResult load_workspace(const std::filesystem::path& root_path);

std::vector<std::filesystem::path> collect_module_interfaces(
    const std::filesystem::path& package_root,
    const std::string& package_name);

std::optional<int> edition_to_standard(const std::string& edition);

}  // namespace renewal
