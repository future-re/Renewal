#include "renewal/workspace.hpp"

#include <algorithm>
#include <filesystem>
#include <queue>
#include <set>
#include <string>

#include "renewal/manifest_parser.hpp"

namespace renewal {

namespace fs = std::filesystem;

namespace {

void add_manifest_path(std::set<fs::path>& manifests, const fs::path& path) {
  manifests.insert(fs::weakly_canonical(path));
}

std::vector<fs::path> discover_seed_manifests(const fs::path& root_path) {
  std::set<fs::path> manifests;
  const fs::path absolute_root = fs::absolute(root_path);
  const fs::path root_manifest = absolute_root / "pkg.toml";
  if (fs::exists(root_manifest)) {
    add_manifest_path(manifests, root_manifest);
  }

  if (!fs::exists(absolute_root) || !fs::is_directory(absolute_root)) {
    return {manifests.begin(), manifests.end()};
  }

  for (const auto& entry : fs::recursive_directory_iterator(absolute_root)) {
    if (!entry.is_regular_file() || entry.path().filename() != "pkg.toml") {
      continue;
    }
    add_manifest_path(manifests, entry.path());
  }

  return {manifests.begin(), manifests.end()};
}

}  // namespace

WorkspaceLoadResult load_workspace(const fs::path& root_path) {
  WorkspaceLoadResult result;
  result.workspace.requested_root = fs::absolute(root_path);

  std::queue<fs::path> pending;
  for (const auto& manifest_path : discover_seed_manifests(root_path)) {
    pending.push(manifest_path);
  }

  std::set<fs::path> visited_manifests;
  std::set<fs::path> added_roots;

  while (!pending.empty()) {
    const fs::path manifest_path = pending.front();
    pending.pop();

    const fs::path canonical_manifest = fs::weakly_canonical(manifest_path);
    if (!visited_manifests.insert(canonical_manifest).second) {
      continue;
    }

    auto parse_result = parse_manifest(canonical_manifest);
    result.diagnostics.insert(result.diagnostics.end(),
                              parse_result.diagnostics.begin(),
                              parse_result.diagnostics.end());

    const fs::path package_root = canonical_manifest.parent_path();
    if (added_roots.insert(package_root).second) {
      result.workspace.packages.push_back(
          PackageInfo{package_root, parse_result.manifest});
    }

    for (const auto& [_, dep] : parse_result.manifest.deps) {
      if (dep.kind != DependencyKind::Path) {
        continue;
      }

      const fs::path dep_path(dep.value);
      if (dep_path.is_absolute()) {
        continue;
      }

      const fs::path dep_manifest = package_root / dep_path / "pkg.toml";
      if (fs::exists(dep_manifest)) {
        pending.push(dep_manifest);
      }
    }
  }

  std::sort(result.workspace.packages.begin(), result.workspace.packages.end(),
            [](const PackageInfo& lhs, const PackageInfo& rhs) {
              const std::string lhs_key = lhs.manifest.package.name.value_or(
                  lhs.root.filename().string());
              const std::string rhs_key = rhs.manifest.package.name.value_or(
                  rhs.root.filename().string());
              if (lhs_key != rhs_key) {
                return lhs_key < rhs_key;
              }
              return lhs.root < rhs.root;
            });

  return result;
}

std::vector<fs::path> collect_module_interfaces(const fs::path& package_root,
                                                const std::string& package_name) {
  std::vector<fs::path> files;
  const fs::path module_root = package_root / "modules" / package_name;
  if (!fs::exists(module_root)) {
    return files;
  }

  for (const auto& entry : fs::recursive_directory_iterator(module_root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".cppm") {
      files.push_back(entry.path());
    }
  }

  std::sort(files.begin(), files.end());
  return files;
}

std::optional<int> edition_to_standard(const std::string& edition) {
  if (edition == "c++20") {
    return 20;
  }
  if (edition == "c++23") {
    return 23;
  }
  if (edition == "c++26") {
    return 26;
  }
  return std::nullopt;
}

}  // namespace renewal
