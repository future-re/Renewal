#include "renewal/manifest_parser.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include "renewal/utils.hpp"

namespace renewal {

using fs = std::filesystem::path;

namespace {

bool is_quoted_string(const std::string& value) {
  return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

std::optional<std::string> parse_string(const std::string& value,
                                        std::vector<Diagnostic>& diagnostics,
                                        const fs& manifest_path,
                                        const std::string& field_name) {
  if (!is_quoted_string(value)) {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "Expected string value for '" + field_name + "'",
                           manifest_path});
    return std::nullopt;
  }
  return value.substr(1, value.size() - 2);
}

std::optional<std::vector<std::string>> parse_string_array(
    const std::string& value, std::vector<Diagnostic>& diagnostics,
    const fs& manifest_path, const std::string& field_name) {
  if (value.size() < 2 || value.front() != '[' || value.back() != ']') {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "Expected string array for '" + field_name + "'",
                           manifest_path});
    return std::nullopt;
  }

  std::vector<std::string> result;
  std::string inner = utils::trim(value.substr(1, value.size() - 2));
  if (inner.empty()) {
    return result;
  }

  std::size_t pos = 0;
  while (pos < inner.size()) {
    while (pos < inner.size() &&
           std::isspace(static_cast<unsigned char>(inner[pos]))) {
      ++pos;
    }
    if (pos >= inner.size()) {
      break;
    }
    if (inner[pos] != '"') {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Expected quoted string item in array for '" + field_name + "'",
           manifest_path});
      return std::nullopt;
    }

    std::size_t end = pos + 1;
    while (end < inner.size()) {
      if (inner[end] == '"' && inner[end - 1] != '\\') {
        break;
      }
      ++end;
    }
    if (end >= inner.size()) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Unterminated string item in array for '" + field_name + "'",
           manifest_path});
      return std::nullopt;
    }

    result.push_back(inner.substr(pos + 1, end - pos - 1));
    pos = end + 1;

    while (pos < inner.size() &&
           std::isspace(static_cast<unsigned char>(inner[pos]))) {
      ++pos;
    }
    if (pos >= inner.size()) {
      break;
    }
    if (inner[pos] != ',') {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Expected ',' between array items for '" + field_name + "'",
           manifest_path});
      return std::nullopt;
    }
    ++pos;
  }

  return result;
}

std::optional<DependencySpec> parse_dependency(
    const std::string& key, const std::string& value,
    std::vector<Diagnostic>& diagnostics, const fs& manifest_path) {
  if (is_quoted_string(value)) {
    auto version = parse_string(value, diagnostics, manifest_path, key);
    if (!version) {
      return std::nullopt;
    }
    return DependencySpec{DependencyKind::Version, *version};
  }

  if (value.size() >= 2 && value.front() == '{' && value.back() == '}') {
    std::string inner = utils::trim(value.substr(1, value.size() - 2));
    std::size_t equals = inner.find('=');
    if (equals == std::string::npos) {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Invalid inline table dependency for '" + key + "'", manifest_path});
      return std::nullopt;
    }

    std::string inline_key = utils::trim(inner.substr(0, equals));
    std::string inline_value = utils::trim(inner.substr(equals + 1));
    if (inline_key != "path") {
      diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Only '{ path = \"...\" }' dependencies are supported for '" + key +
               "'",
           manifest_path});
      return std::nullopt;
    }

    auto path_value =
        parse_string(inline_value, diagnostics, manifest_path, key + ".path");
    if (!path_value) {
      return std::nullopt;
    }
    return DependencySpec{DependencyKind::Path, *path_value};
  }

  diagnostics.push_back({DiagnosticSeverity::Error,
                         "Unsupported dependency value for '" + key + "'",
                         manifest_path});
  return std::nullopt;
}

bool assign_package_field(PackageManifest& manifest, const std::string& key,
                          const std::string& value,
                          std::vector<Diagnostic>& diagnostics,
                          const fs& manifest_path) {
  auto parsed =
      parse_string(value, diagnostics, manifest_path, "package." + key);
  if (!parsed) {
    return false;
  }

  if (key == "name") {
    manifest.package.name = *parsed;
  } else if (key == "version") {
    manifest.package.version = *parsed;
  } else if (key == "edition") {
    manifest.package.edition = *parsed;
  } else if (key == "description") {
    manifest.package.description = *parsed;
  } else if (key == "license") {
    manifest.package.license = *parsed;
  } else if (key == "repository") {
    manifest.package.repository = *parsed;
  } else {
    diagnostics.push_back({DiagnosticSeverity::Error,
                           "Unknown field 'package." + key + "'",
                           manifest_path});
    return false;
  }

  return true;
}

bool assign_build_field(PackageManifest& manifest, const std::string& key,
                        const std::string& value,
                        std::vector<Diagnostic>& diagnostics,
                        const fs& manifest_path) {
  if (key == "target" || key == "type") {
    auto parsed =
        parse_string(value, diagnostics, manifest_path, "build." + key);
    if (!parsed) {
      return false;
    }

    if (key == "target") {
      manifest.build.target = *parsed;
    } else {
      manifest.build.type = *parsed;
    }
    return true;
  }

  if (key == "sources") {
    auto parsed =
        parse_string_array(value, diagnostics, manifest_path, "build.sources");
    if (!parsed) {
      return false;
    }
    manifest.build.sources = std::move(*parsed);
    return true;
  }

  diagnostics.push_back({DiagnosticSeverity::Error,
                         "Unknown field 'build." + key + "'", manifest_path});
  return false;
}

void add_required_field_diagnostics(const PackageManifest& manifest,
                                    std::vector<Diagnostic>& diagnostics,
                                    const fs& manifest_path) {
  auto require_string = [&](const std::optional<std::string>& field,
                            const std::string& name) {
    if (!field || field->empty()) {
      diagnostics.push_back({DiagnosticSeverity::Error,
                             "Missing required field '" + name + "'",
                             manifest_path});
    }
  };

  require_string(manifest.package.name, "package.name");
  require_string(manifest.package.version, "package.version");
  require_string(manifest.package.edition, "package.edition");
  require_string(manifest.build.target, "build.target");
  require_string(manifest.build.type, "build.type");
}

}  // namespace

ManifestParseResult parse_manifest(const std::filesystem::path& manifest_path) {
  ManifestParseResult result;

  std::ifstream input(manifest_path);
  if (!input) {
    result.diagnostics.push_back(
        {DiagnosticSeverity::Error,
         "Failed to open manifest: " + manifest_path.string(), manifest_path});
    return result;
  }

  std::unordered_set<std::string> allowed_sections = {"package", "deps",
                                                      "build", "metadata"};
  std::string section;
  std::string line;
  int line_number = 0;

  while (std::getline(input, line)) {
    ++line_number;
    std::string current = utils::trim(utils::strip_comment(line));
    if (current.empty()) {
      continue;
    }

    if (current.front() == '[' && current.back() == ']') {
      section = utils::trim(current.substr(1, current.size() - 2));
      if (!allowed_sections.count(section)) {
        result.diagnostics.push_back(
            {DiagnosticSeverity::Error,
             "Unknown top-level table [" + section + "]", manifest_path});
        section.clear();
      }
      continue;
    }

    if (section.empty()) {
      result.diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Key-value entry must appear inside a supported table",
           manifest_path});
      continue;
    }

    std::size_t equals = current.find('=');
    if (equals == std::string::npos) {
      result.diagnostics.push_back(
          {DiagnosticSeverity::Error,
           "Invalid entry at line " + std::to_string(line_number),
           manifest_path});
      continue;
    }

    std::string key = utils::trim(current.substr(0, equals));
    std::string value = utils::trim(current.substr(equals + 1));

    if (!value.empty() && value.front() == '[' && value.back() != ']') {
      std::ostringstream continued;
      continued << value;
      while (std::getline(input, line)) {
        ++line_number;
        std::string next = utils::trim(utils::strip_comment(line));
        if (!next.empty()) {
          continued << ' ' << next;
        }
        if (!next.empty() && next.back() == ']') {
          break;
        }
      }
      value = continued.str();
    }

    if (section == "package") {
      assign_package_field(result.manifest, key, value, result.diagnostics,
                           manifest_path);
      continue;
    }

    if (section == "deps") {
      auto dep =
          parse_dependency(key, value, result.diagnostics, manifest_path);
      if (dep) {
        result.manifest.deps[key] = *dep;
      }
      continue;
    }

    if (section == "build") {
      assign_build_field(result.manifest, key, value, result.diagnostics,
                         manifest_path);
      continue;
    }

    result.manifest.metadata[key] = value;
  }

  add_required_field_diagnostics(result.manifest, result.diagnostics,
                                 manifest_path);
  return result;
}

}  // namespace renewal
