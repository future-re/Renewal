#include "renewal/generator.hpp"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "renewal/utils.hpp"
#include "renewal/validator.hpp"
#include "renewal/workspace.hpp"

namespace renewal {

namespace fs = std::filesystem;

namespace {

std::string quote_cmake_string(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    if (ch == '\\') {
      escaped += "\\\\";
    } else if (ch == '"') {
      escaped += "\\\"";
    } else {
      escaped += ch;
    }
  }
  return "\"" + escaped + "\"";
}

std::string quote_cmake_path(const fs::path& path) {
  return quote_cmake_string(path.generic_string());
}

std::string sanitize_project_name(const std::string& value) {
  std::string result;
  result.reserve(value.size());
  for (char ch : value) {
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
        (ch >= '0' && ch <= '9') || ch == '_') {
      result.push_back(ch);
    } else {
      result.push_back('_');
    }
  }

  if (result.empty()) {
    return "renewal_workspace";
  }
  if (result.front() >= '0' && result.front() <= '9') {
    result.insert(result.begin(), '_');
  }
  return result;
}

std::string project_name_for(const WorkspaceInfo& workspace) {
  if (workspace.packages.size() == 1 &&
      workspace.packages.front().manifest.package.name) {
    return sanitize_project_name(
        *workspace.packages.front().manifest.package.name);
  }

  return sanitize_project_name(
      workspace.requested_root.filename().string().empty()
          ? std::string("renewal_workspace")
          : workspace.requested_root.filename().string());
}

std::expected<int, std::string> workspace_standard_for(
    const WorkspaceInfo& workspace) {
  std::optional<int> standard;

  for (const auto& package : workspace.packages) {
    if (!package.manifest.package.edition) {
      return std::unexpected("Missing package.edition in " +
                             (package.root / "pkg.toml").string());
    }

    const auto mapped = edition_to_standard(*package.manifest.package.edition);
    if (!mapped) {
      return std::unexpected("Unsupported package.edition '" +
                             *package.manifest.package.edition + "' in " +
                             (package.root / "pkg.toml").string());
    }

    if (!standard || *mapped > *standard) {
      standard = *mapped;
    }
  }

  if (!standard) {
    return std::unexpected("No packages found to derive C++ standard");
  }
  return *standard;
}

std::optional<std::string> unresolved_version_dependencies(
    const WorkspaceInfo& workspace) {
  std::vector<std::string> failures;
  for (const auto& package : workspace.packages) {
    const std::string package_name =
        package.manifest.package.name.value_or(package.root.filename().string());
    for (const auto& [dep_name, dep] : package.manifest.deps) {
      if (dep.kind == DependencyKind::Version) {
        failures.push_back(package_name + ": " + dep_name + " = " + dep.value);
      }
    }
  }

  if (failures.empty()) {
    return std::nullopt;
  }

  std::sort(failures.begin(), failures.end());

  std::ostringstream message;
  message << "Version-based dependencies are not implemented yet:\n";
  for (const auto& failure : failures) {
    message << "  " << failure << "\n";
  }
  return message.str();
}

std::set<std::string> local_dependency_targets(
    const PackageInfo& package, const WorkspaceInfo& workspace) {
  std::set<std::string> targets;

  for (const auto& [dep_name, dep] : package.manifest.deps) {
    if (dep.kind != DependencyKind::Path) {
      continue;
    }

    for (const auto& candidate : workspace.packages) {
      if (!candidate.manifest.package.name ||
          !candidate.manifest.build.target) {
        continue;
      }
      if (*candidate.manifest.package.name == dep_name) {
        targets.insert(*candidate.manifest.build.target);
      }
    }
  }

  return targets;
}

fs::path relative_from_generated(const GeneratedLayout& layout,
                                 const fs::path& source_path) {
  return fs::relative(source_path, layout.cmake_source_dir);
}

}  // namespace

GeneratedLayout generated_layout_for(const fs::path& root_path) {
  const fs::path absolute_root = fs::absolute(root_path);
  const fs::path tool_root = absolute_root / ".renewal";
  return GeneratedLayout{
      absolute_root,
      tool_root,
      tool_root / "cmake-src",
      tool_root / "build",
      tool_root / "cmake-src" / "CMakeLists.txt",
  };
}

std::expected<GeneratedProject, std::string> generate_cmake(
    const fs::path& root_path) {
  auto validation = validator(root_path);
  if (!validation) {
    return std::unexpected(validation.error());
  }

  auto workspace_result = load_workspace(root_path);
  if (!workspace_result.ok()) {
    return std::unexpected("Workspace became invalid during generation");
  }

  if (workspace_result.workspace.packages.empty()) {
    return std::unexpected("No pkg.toml found under: " + root_path.string());
  }

  if (const auto unresolved =
          unresolved_version_dependencies(workspace_result.workspace)) {
    return std::unexpected(*unresolved);
  }

  auto workspace_standard = workspace_standard_for(workspace_result.workspace);
  if (!workspace_standard) {
    return std::unexpected(workspace_standard.error());
  }

  GeneratedProject project;
  project.layout = generated_layout_for(root_path);

  std::ostringstream output;
  output << "cmake_minimum_required(VERSION 3.28)\n\n";
  output << "project("
         << sanitize_project_name(project_name_for(workspace_result.workspace))
         << " LANGUAGES CXX)\n\n";
  output << "set(CMAKE_CXX_STANDARD " << *workspace_standard << ")\n";
  output << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n";
  output << "set(CMAKE_CXX_EXTENSIONS OFF)\n\n";

  for (const auto& package : workspace_result.workspace.packages) {
    if (!package.manifest.package.name || !package.manifest.build.target ||
        !package.manifest.package.edition) {
      return std::unexpected("Package metadata missing for generated build at " +
                             (package.root / "pkg.toml").string());
    }

    const auto package_standard =
        edition_to_standard(*package.manifest.package.edition);
    if (!package_standard) {
      return std::unexpected("Unsupported package.edition '" +
                             *package.manifest.package.edition + "'");
    }

    const auto interfaces =
        collect_module_interfaces(package.root, *package.manifest.package.name);
    if (interfaces.empty()) {
      return std::unexpected("No module interface units found for package '" +
                             *package.manifest.package.name + "'");
    }

    output << "add_library(" << *package.manifest.build.target << ")\n";
    output << "set_target_properties(" << *package.manifest.build.target
           << " PROPERTIES\n";
    output << "  CXX_STANDARD " << *package_standard << "\n";
    output << "  CXX_STANDARD_REQUIRED ON\n";
    output << "  CXX_EXTENSIONS OFF\n";
    output << ")\n";
    output << "target_sources(" << *package.manifest.build.target << "\n";
    output << "  PUBLIC\n";
    output << "    FILE_SET cxxmods TYPE CXX_MODULES\n";
    output << "    BASE_DIRS "
           << quote_cmake_path(relative_from_generated(
                  project.layout,
                  package.root / "modules" / *package.manifest.package.name))
           << "\n";
    output << "    FILES\n";
    for (const auto& file : interfaces) {
      output << "      "
             << quote_cmake_path(relative_from_generated(project.layout, file))
             << "\n";
    }

    if (!package.manifest.build.sources.empty()) {
      output << "  PRIVATE\n";
      for (const auto& source : package.manifest.build.sources) {
        output << "      "
               << quote_cmake_path(relative_from_generated(
                      project.layout, package.root / source))
               << "\n";
      }
    }
    output << ")\n";

    const auto deps =
        local_dependency_targets(package, workspace_result.workspace);
    if (!deps.empty()) {
      output << "target_link_libraries(" << *package.manifest.build.target
             << "\n";
      output << "  PUBLIC\n";
      for (const auto& dep_target : deps) {
        output << "    " << dep_target << "\n";
      }
      output << ")\n";
    }

    output << "\n";
  }

  project.cmake_lists = output.str();
  return project;
}

std::expected<GeneratedLayout, std::string> write_generated_project(
    const fs::path& root_path) {
  auto generated = generate_cmake(root_path);
  if (!generated) {
    return std::unexpected(generated.error());
  }

  try {
    fs::create_directories(generated->layout.cmake_source_dir);
    fs::create_directories(generated->layout.build_dir);
    utils::write_file(generated->layout.cmake_lists_path,
                      generated->cmake_lists);
  } catch (const std::exception& error) {
    return std::unexpected(error.what());
  }

  return generated->layout;
}

}  // namespace renewal
