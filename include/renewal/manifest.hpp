#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace renewal {

enum class DependencyKind {
  Version,
  Path,
};

struct DependencySpec {
  DependencyKind kind = DependencyKind::Version;
  std::string value;
};

struct PackageSection {
  std::optional<std::string> name;
  std::optional<std::string> version;
  std::optional<std::string> edition;
  std::optional<std::string> description;
  std::optional<std::string> license;
  std::optional<std::string> repository;
};

struct BuildSection {
  std::optional<std::string> target;
  std::optional<std::string> type;
  std::vector<std::string> sources;
};

struct PackageManifest {
  PackageSection package;
  std::unordered_map<std::string, DependencySpec> deps;
  BuildSection build;
  std::unordered_map<std::string, std::string> metadata;
};

}  // namespace renewal
