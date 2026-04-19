#pragma once

#include "renewal/diagnostic.hpp"
#include "renewal/manifest.hpp"

#include <filesystem>
#include <vector>

namespace renewal {

struct ManifestParseResult {
  PackageManifest manifest;
  std::vector<Diagnostic> diagnostics;

  [[nodiscard]] bool ok() const { return diagnostics.empty(); }
};

ManifestParseResult parse_manifest(const std::filesystem::path& manifest_path);

}  // namespace renewal
