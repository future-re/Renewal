#include "renewal/build.hpp"

#include <cstdlib>
#include <expected>
#include <filesystem>
#include <sstream>
#include <string>

#include "renewal/generator.hpp"

namespace renewal {

namespace fs = std::filesystem;

namespace {

std::string shell_quote(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    if (ch == '"' || ch == '\\') {
      escaped.push_back('\\');
    }
    escaped.push_back(ch);
  }
  return "\"" + escaped + "\"";
}

std::string shell_quote(const fs::path& value) {
  return shell_quote(value.string());
}

bool command_available(const std::string& command) {
  const std::string probe = command + " >/dev/null 2>&1";
  return std::system(probe.c_str()) == 0;
}

std::string configure_command_for(const GeneratedLayout& layout) {
  std::ostringstream command;
  command << "cmake -S " << shell_quote(layout.cmake_source_dir) << " -B "
          << shell_quote(layout.build_dir);

  const char* configured_generator = std::getenv("CMAKE_GENERATOR");
  if ((!configured_generator || *configured_generator == '\0') &&
      command_available("ninja --version")) {
    command << " -G Ninja";
  }

  return command.str();
}

std::string build_command_for(const GeneratedLayout& layout) {
  std::ostringstream command;
  command << "cmake --build " << shell_quote(layout.build_dir);
  return command.str();
}

}  // namespace

std::expected<GeneratedLayout, std::string> build_project(
    const fs::path& root_path) {
  if (!command_available("cmake --version")) {
    return std::unexpected("cmake executable not found in PATH");
  }

  auto layout = write_generated_project(root_path);
  if (!layout) {
    return std::unexpected(layout.error());
  }

  const std::string configure_command = configure_command_for(*layout);
  if (std::system(configure_command.c_str()) != 0) {
    return std::unexpected("CMake configure failed: " + configure_command);
  }

  const std::string build_command = build_command_for(*layout);
  if (std::system(build_command.c_str()) != 0) {
    return std::unexpected("CMake build failed: " + build_command);
  }

  return *layout;
}

}  // namespace renewal
