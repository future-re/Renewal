#include "renewal/build.hpp"
#include "renewal/create_new.hpp"
#include "renewal/generator.hpp"
#include "renewal/validator.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void print_usage() {
  std::cerr << "Usage: renewal <command> [path] [--stdout]" << std::endl;
  std::cerr << "Commands:" << std::endl;
  std::cerr << "  validate   Validate a package or workspace" << std::endl;
  std::cerr << "  generate   Write generated CMake project under .renewal"
            << std::endl;
  std::cerr << "  build      Validate, generate, configure, and build"
            << std::endl;
  std::cerr << "  new        Create a new package directory" << std::endl;
}

int handle_validate(const std::filesystem::path& path) {
  auto result = renewal::validator(path);
  if (!result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  std::cout << "Validation passed" << std::endl;
  return 0;
}

bool has_flag(int argc, char** argv, const std::string& flag) {
  for (int i = 2; i < argc; ++i) {
    if (flag == argv[i]) {
      return true;
    }
  }
  return false;
}

std::filesystem::path resolve_path_argument(int argc, char** argv,
                                            const std::filesystem::path& fallback) {
  for (int i = 2; i < argc; ++i) {
    const std::string value = argv[i];
    if (!value.empty() && value[0] == '-') {
      continue;
    }
    return value;
  }
  return fallback;
}

int handle_generate(const std::filesystem::path& path, bool write_stdout) {
  auto result = renewal::generate_cmake(path);
  if (!result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (write_stdout) {
    std::cout << result->cmake_lists;
    return 0;
  }

  auto written = renewal::write_generated_project(path);
  if (!written) {
    std::cerr << written.error() << std::endl;
    return 1;
  }

  std::cout << "Generated CMake project at " << written->cmake_lists_path
            << std::endl;
  return 0;
}

int handle_build(const std::filesystem::path& path) {
  auto result = renewal::build_project(path);
  if (!result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  std::cout << renewal::format_toolchain_report(result->toolchain);
  std::cout << "Build completed in " << result->layout.build_dir << std::endl;
  return 0;
}

int handle_new(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: renewal new <path>" << std::endl;
    return 2;
  }

  auto result = renewal::create_new(argv[2]);
  if (!result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  std::cout << "Created package at " << std::filesystem::path(argv[2]).string()
            << std::endl;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    print_usage();
    return 2;
  }

  const std::filesystem::path current_path = std::filesystem::current_path();
  const std::string command = argv[1];
  const std::filesystem::path path =
      resolve_path_argument(argc, argv, current_path);

  if (command == "validate") {
    return handle_validate(path);
  }
  if (command == "generate") {
    return handle_generate(path, has_flag(argc, argv, "--stdout"));
  }
  if (command == "build") {
    return handle_build(path);
  }
  if (command == "new") {
    return handle_new(argc, argv);
  }

  std::cerr << "Unknown command: " << command << std::endl;
  print_usage();
  return 2;
}
