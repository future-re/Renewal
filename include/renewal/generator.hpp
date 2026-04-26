#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace renewal {

struct GeneratedLayout {
  std::filesystem::path root;
  std::filesystem::path tool_root;
  std::filesystem::path cmake_source_dir;
  std::filesystem::path build_dir;
  std::filesystem::path cmake_lists_path;
};

struct GeneratedProject {
  GeneratedLayout layout;
  std::string cmake_lists;
};

GeneratedLayout generated_layout_for(const std::filesystem::path& root_path);

std::expected<GeneratedProject, std::string> generate_cmake(
    const std::filesystem::path& root_path);

std::expected<GeneratedLayout, std::string> write_generated_project(
    const std::filesystem::path& root_path);

}  // namespace renewal
