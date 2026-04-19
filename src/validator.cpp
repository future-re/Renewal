#include "renewal/validator.hpp"

#include <filesystem>
#include <expected>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>
#include "renewal/manifest_parser.hpp"

namespace renewal {

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }

  std::ostringstream stream;
  stream << input.rdbuf();
  return stream.str();
}

std::string trim(const std::string& input) {
  std::size_t begin = input.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  std::size_t end = input.find_last_not_of(" \t\r\n");
  return input.substr(begin, end - begin + 1);
}

bool is_valid_package_name(const std::string& name) {
  static const std::regex pattern("^[A-Za-z0-9_-]+$");
  return std::regex_match(name, pattern);
}

bool is_valid_semver(const std::string& version) {
  static const std::regex pattern("^[0-9]+\\.[0-9]+\\.[0-9]+$");
  return std::regex_match(version, pattern);
}

std::string dependency_key_from_import(const std::string& imported) {
  std::size_t pos = imported.find_first_of(".:");
  if (pos == std::string::npos) {
    return imported;
  }
  return imported.substr(0, pos);
}

}  // namespace

std::vector<Diagnostic> validate_package(
    const std::filesystem::path& package_root,
    const PackageManifest& manifest) {
  std::vector<Diagnostic> diagnostics;

  if (manifest.package.name && !manifest.package.name->empty() &&
      !is_valid_package_name(*manifest.package.name)) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "package.name may only contain [A-Za-z0-9_-]",
                           package_root / "pkg.toml"});
  }

  if (manifest.package.version && !manifest.package.version->empty() &&
      !is_valid_semver(*manifest.package.version)) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "package.version must match MAJOR.MINOR.PATCH",
                           package_root / "pkg.toml"});
  }

  if (manifest.package.edition && !manifest.package.edition->empty()) {
    static const std::unordered_set<std::string> allowed_editions = {
        "c++20", "c++23", "c++26"};
    if (!allowed_editions.count(*manifest.package.edition)) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "package.edition must be one of c++20, c++23, c++26",
           package_root / "pkg.toml"});
    }
  }

  if (manifest.build.type && !manifest.build.type->empty() &&
      *manifest.build.type != "module_library") {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "build.type must be 'module_library'",
                           package_root / "pkg.toml"});
  }

  for (const auto& [name, dep] : manifest.deps) {
    if (dep.kind != DependencyKind::Path) {
      continue;
    }

    std::filesystem::path dep_path(dep.value);
    if (dep_path.is_absolute()) {
      diagnostics.push_back({DiagnosticSeverity::Error,
                             "Dependency '" + name + "' path must be relative",
                             package_root / "pkg.toml"});
      continue;
    }

    std::filesystem::path target_manifest =
        package_root / dep_path / "pkg.toml";
    if (!std::filesystem::exists(target_manifest)) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Dependency '" + name +
               "' path must point to a package root containing pkg.toml",
           package_root / "pkg.toml"});
    }
  }

  if (!manifest.package.name || manifest.package.name->empty()) {
    return diagnostics;
  }

  std::filesystem::path module_path =
      package_root / "modules" / *manifest.package.name / "mod.cppm";
  if (!std::filesystem::exists(module_path)) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "Missing unified export module at " + module_path.string(),
         module_path});
    return diagnostics;
  }

  std::string module_source = read_file(module_path);
  if (module_source.empty()) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "Failed to read module file: " + module_path.string(), module_path});
    return diagnostics;
  }

  static const std::regex export_module_pattern(
      R"(export\s+module\s+([A-Za-z0-9_.:-]+)\s*;)");
  std::smatch export_match;
  if (!std::regex_search(module_source, export_match, export_module_pattern)) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "mod.cppm must contain 'export module <package.name>;'", module_path});
  } else if (trim(export_match[1].str()) != *manifest.package.name) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "export module declaration must match package.name",
                           module_path});
  }

  static const std::regex import_pattern(
      R"(\bimport\s+([A-Za-z_][A-Za-z0-9_.:-]*)\s*;)");
  for (std::sregex_iterator
           it(module_source.begin(), module_source.end(), import_pattern),
       end;
       it != end; ++it) {
    std::string imported = (*it)[1].str();
    if (imported == "std") {
      continue;
    }

    std::string dep_name = dependency_key_from_import(imported);
    if (dep_name.empty() || dep_name == *manifest.package.name) {
      continue;
    }

    if (!manifest.deps.count(dep_name)) {
      diagnostics.push_back({DiagnosticSeverity::Error,
                             "Module import '" + imported +
                                 "' is not declared in [deps] as package '" +
                                 dep_name + "'",
                             module_path});
    }
  }

  return diagnostics;
}

void print_diagnostics(const std::vector<renewal::Diagnostic>& diagnostics) {
  for (const auto& diagnostic : diagnostics) {
    std::cerr << "error";
    if (!diagnostic.path.empty()) {
      std::cerr << ": " << diagnostic.path.string();
    }
    std::cerr << ": " << diagnostic.message << std::endl;
  }
}

std::expected<void, std::string> validator(std::filesystem::path package_root) {
  std::filesystem::path manifest_path = package_root / "pkg.toml";

  if (!std::filesystem::exists(package_root)) {
    return std::unexpected("Package root not found: " + package_root.string());
  }

  if (!std::filesystem::exists(manifest_path)) {
    return std::unexpected("Manifest not found: " + manifest_path.string());
  }

  auto parse_result = renewal::parse_manifest(manifest_path);
  if (!parse_result.ok()) {
    print_diagnostics(parse_result.diagnostics);
  }

  auto diagnostics = renewal::validate_package(
      std::filesystem::absolute(package_root), parse_result.manifest);
  if (!diagnostics.empty()) {
    print_diagnostics(diagnostics);
  }
  return {};
}
}  // namespace renewal
