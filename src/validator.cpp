#include "renewal/validator.hpp"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "renewal/manifest_parser.hpp"
#include "renewal/utils.hpp"
#include "renewal/workspace.hpp"

namespace renewal {

namespace fs = std::filesystem;

namespace {

struct PackageBundle {
  fs::path root;
  PackageManifest manifest;
};

bool is_valid_package_name(const std::string& name) {
  static const std::regex pattern("^[A-Za-z0-9_-]+$");
  return std::regex_match(name, pattern);
}

bool is_valid_semver(const std::string& version) {
  static const std::regex pattern("^[0-9]+\\.[0-9]+\\.[0-9]+$");
  return std::regex_match(version, pattern);
}

std::string dependency_key_from_import(const std::string& imported) {
  if (imported.empty() || imported.front() == ':') {
    return "";
  }

  const std::size_t pos = imported.find_first_of(".:");
  if (pos == std::string::npos) {
    return imported;
  }
  return imported.substr(0, pos);
}

std::vector<fs::path> collect_sources_to_scan(const fs::path& package_root,
                                              const PackageManifest& manifest) {
  std::vector<fs::path> files;
  if (!manifest.package.name) {
    return files;
  }

  auto interfaces =
      collect_module_interfaces(package_root, *manifest.package.name);
  files.insert(files.end(), interfaces.begin(), interfaces.end());

  for (const auto& source : manifest.build.sources) {
    files.push_back(package_root / source);
  }

  std::sort(files.begin(), files.end());
  files.erase(std::unique(files.begin(), files.end()), files.end());
  return files;
}

std::vector<std::string> collect_imports(const std::string& source) {
  std::vector<std::string> imports;
  static const std::regex import_pattern(
      R"(\bimport\s+([:<"][^;]+|[A-Za-z_][A-Za-z0-9_.:-]*)\s*;)");

  for (std::sregex_iterator it(source.begin(), source.end(), import_pattern),
       end;
       it != end; ++it) {
    imports.push_back(utils::trim((*it)[1].str()));
  }

  return imports;
}

void print_diagnostics(const std::vector<Diagnostic>& diagnostics) {
  for (const auto& diagnostic : diagnostics) {
    std::cerr << (diagnostic.severity == DiagnosticSeverity::Error ? "error"
                                                                   : "warning");
    if (!diagnostic.path.empty()) {
      std::cerr << ": " << diagnostic.path.string();
    }
    std::cerr << ": " << diagnostic.message << std::endl;
  }
}

void append(std::vector<Diagnostic>& target,
            const std::vector<Diagnostic>& source) {
  target.insert(target.end(), source.begin(), source.end());
}

std::optional<std::string> detect_cycle(
    const std::unordered_map<std::string, std::set<std::string>>& graph) {
  enum class VisitState {
    Unvisited,
    Visiting,
    Done,
  };

  std::unordered_map<std::string, VisitState> states;
  std::vector<std::string> stack;

  std::function<std::optional<std::string>(const std::string&)> dfs =
      [&](const std::string& node) -> std::optional<std::string> {
    states[node] = VisitState::Visiting;
    stack.push_back(node);

    auto graph_it = graph.find(node);
    if (graph_it != graph.end()) {
      for (const auto& next : graph_it->second) {
        if (states[next] == VisitState::Visiting) {
          std::ostringstream cycle;
          auto begin = std::find(stack.begin(), stack.end(), next);
          for (auto it = begin; it != stack.end(); ++it) {
            if (it != begin) {
              cycle << " -> ";
            }
            cycle << *it;
          }
          cycle << " -> " << next;
          return cycle.str();
        }

        if (states[next] == VisitState::Unvisited) {
          auto result = dfs(next);
          if (result) {
            return result;
          }
        }
      }
    }

    stack.pop_back();
    states[node] = VisitState::Done;
    return std::nullopt;
  };

  std::vector<std::string> nodes;
  nodes.reserve(graph.size());
  for (const auto& [node, _] : graph) {
    nodes.push_back(node);
  }
  std::sort(nodes.begin(), nodes.end());

  for (const auto& node : nodes) {
    if (states[node] != VisitState::Unvisited) {
      continue;
    }
    auto cycle = dfs(node);
    if (cycle) {
      return cycle;
    }
  }

  return std::nullopt;
}

}  // namespace

std::vector<Diagnostic> validate_package(
    const std::filesystem::path& package_root,
    const PackageManifest& manifest) {
  std::vector<Diagnostic> diagnostics;
  const fs::path manifest_path = package_root / "pkg.toml";

  if (manifest.package.name && !manifest.package.name->empty() &&
      !is_valid_package_name(*manifest.package.name)) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "package.name may only contain [A-Za-z0-9_-]",
                           manifest_path});
  }

  if (manifest.package.version && !manifest.package.version->empty() &&
      !is_valid_semver(*manifest.package.version)) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "package.version must match MAJOR.MINOR.PATCH",
                           manifest_path});
  }

  if (manifest.package.edition && !manifest.package.edition->empty()) {
    static const std::unordered_set<std::string> allowed_editions = {
        "c++20", "c++23", "c++26"};
    if (!allowed_editions.count(*manifest.package.edition)) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "package.edition must be one of c++20, c++23, c++26",
           manifest_path});
    }
  }

  if (manifest.build.type && !manifest.build.type->empty() &&
      *manifest.build.type != "module_library") {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "build.type must be 'module_library'",
                           manifest_path});
  }

  for (const auto& [name, dep] : manifest.deps) {
    if (dep.kind == DependencyKind::Version && !is_valid_semver(dep.value)) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Dependency '" + name + "' version must match MAJOR.MINOR.PATCH",
           manifest_path});
    }

    if (dep.kind != DependencyKind::Path) {
      continue;
    }

    const fs::path dep_path(dep.value);
    if (dep_path.is_absolute()) {
      diagnostics.push_back({DiagnosticSeverity::Error,
                             "Dependency '" + name + "' path must be relative",
                             manifest_path});
      continue;
    }

    const fs::path dep_manifest = package_root / dep_path / "pkg.toml";
    if (!fs::exists(dep_manifest)) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Dependency '" + name +
               "' path must point to a package root containing pkg.toml",
           manifest_path});
      continue;
    }

    auto dep_parse_result = parse_manifest(dep_manifest);
    if (dep_parse_result.ok() && dep_parse_result.manifest.package.name &&
        *dep_parse_result.manifest.package.name != name) {
      diagnostics.push_back({DiagnosticSeverity::Error,
                             "Dependency key '" + name +
                                 "' must match target package name '" +
                                 *dep_parse_result.manifest.package.name + "'",
                             manifest_path});
    }
  }

  if (!manifest.package.name || manifest.package.name->empty()) {
    return diagnostics;
  }

  const fs::path module_root = package_root / "modules" / *manifest.package.name;
  const fs::path unified_module = module_root / "mod.cppm";
  if (!fs::exists(unified_module)) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "Missing unified export module at " + unified_module.string(),
         unified_module});
    return diagnostics;
  }

  const auto module_interfaces =
      collect_module_interfaces(package_root, *manifest.package.name);
  if (module_interfaces.empty()) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "Package must contain at least one module interface unit",
         module_root});
  }

  for (const auto& source : manifest.build.sources) {
    const fs::path source_path = package_root / source;
    if (fs::path(source).is_absolute()) {
      diagnostics.push_back({DiagnosticSeverity::Error,
                             "build.sources entries must be relative paths",
                             manifest_path});
      continue;
    }

    if (!fs::exists(source_path)) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "build.sources entry does not exist: " + source, manifest_path});
    }
  }

  const std::string unified_source = utils::read_file(unified_module);
  static const std::regex export_module_pattern(
      R"(export\s+module\s+([A-Za-z0-9_.:-]+)\s*;)");
  std::smatch export_match;
  if (!std::regex_search(unified_source, export_match, export_module_pattern)) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "mod.cppm must contain 'export module <package.name>;'",
         unified_module});
  } else if (utils::trim(export_match[1].str()) != *manifest.package.name) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "export module declaration must match package.name",
                           unified_module});
  }

  for (const auto& file : collect_sources_to_scan(package_root, manifest)) {
    if (!fs::exists(file)) {
      continue;
    }

    const std::string source = utils::read_file(file);
    for (const auto& imported : collect_imports(source)) {
      if (!imported.empty() &&
          (imported.front() == '"' || imported.front() == '<')) {
        diagnostics.push_back({DiagnosticSeverity::Error,
                               "Header units are forbidden in v0.1",
                               file});
        continue;
      }

      if (imported == "std") {
        diagnostics.push_back({DiagnosticSeverity::Error,
                               "import std is forbidden in v0.1", file});
        continue;
      }

      const std::string dep_name = dependency_key_from_import(imported);
      if (dep_name.empty() || dep_name == *manifest.package.name) {
        continue;
      }

      if (!manifest.deps.count(dep_name)) {
        diagnostics.push_back({DiagnosticSeverity::Error,
                               "Module import '" + imported +
                                   "' is not declared in [deps] as package '" +
                                   dep_name + "'",
                               file});
      }
    }
  }

  return diagnostics;
}

std::expected<void, std::string> validator(const fs::path& package_root) {
  if (!fs::exists(package_root)) {
    return std::unexpected("Path not found: " + package_root.string());
  }

  const auto workspace_result = load_workspace(package_root);
  if (workspace_result.workspace.packages.empty()) {
    return std::unexpected("No pkg.toml found under: " + package_root.string());
  }

  std::vector<Diagnostic> diagnostics;
  append(diagnostics, workspace_result.diagnostics);
  std::vector<PackageBundle> packages;
  packages.reserve(workspace_result.workspace.packages.size());
  for (const auto& package : workspace_result.workspace.packages) {
    packages.push_back({package.root, package.manifest});
  }

  for (const auto& package : packages) {
    append(diagnostics, validate_package(package.root, package.manifest));
  }

  std::unordered_map<std::string, fs::path> package_names;
  std::unordered_map<std::string, fs::path> target_names;
  std::unordered_map<std::string, std::set<std::string>> dependency_graph;

  for (const auto& package : packages) {
    if (package.manifest.package.name) {
      const auto [it, inserted] =
          package_names.emplace(*package.manifest.package.name, package.root);
      if (!inserted) {
        diagnostics.push_back(
            {DiagnosticSeverity::Error,
             "Duplicate package.name '" + *package.manifest.package.name +
                 "' found in workspace",
             package.root / "pkg.toml"});
        diagnostics.push_back(
            {DiagnosticSeverity::Error,
             "First package.name '" + *package.manifest.package.name +
                 "' defined here",
             it->second / "pkg.toml"});
      }
    }

    if (package.manifest.build.target) {
      const auto [it, inserted] =
          target_names.emplace(*package.manifest.build.target, package.root);
      if (!inserted) {
        diagnostics.push_back(
            {DiagnosticSeverity::Error,
             "Duplicate build.target '" + *package.manifest.build.target +
                 "' found in workspace",
             package.root / "pkg.toml"});
        diagnostics.push_back(
            {DiagnosticSeverity::Error,
             "First build.target '" + *package.manifest.build.target +
                 "' defined here",
             it->second / "pkg.toml"});
      }
    }
  }

  for (const auto& package : packages) {
    if (!package.manifest.package.name) {
      continue;
    }

    auto& edges = dependency_graph[*package.manifest.package.name];
    for (const auto& [dep_name, dep] : package.manifest.deps) {
      if (dep.kind == DependencyKind::Path) {
        const fs::path dep_manifest = package.root / dep.value / "pkg.toml";
        auto parse_result = parse_manifest(dep_manifest);
        if (parse_result.ok() && parse_result.manifest.package.name) {
          edges.insert(*parse_result.manifest.package.name);
          continue;
        }
      }

      if (package_names.count(dep_name)) {
        edges.insert(dep_name);
      }
    }
  }

  if (auto cycle = detect_cycle(dependency_graph)) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "Cyclic package dependency detected: " + *cycle, package_root});
  }

  if (!diagnostics.empty()) {
    print_diagnostics(diagnostics);
    return std::unexpected("Validation failed");
  }

  return {};
}

}  // namespace renewal
