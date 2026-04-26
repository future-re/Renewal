#include "renewal/create_new.hpp"
#include "renewal/generator.hpp"
#include "renewal/validator.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void print_usage() {
  std::cerr << "Usage: renewal <command> [path]" << std::endl;
  std::cerr << "Commands:" << std::endl;
  std::cerr << "  validate   Validate a package or workspace" << std::endl;
  std::cerr << "  generate   Print generated CMake mapping" << std::endl;
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

int handle_generate(const std::filesystem::path& path) {
  auto result = renewal::generate_cmake(path);
  if (!result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  std::cout << *result;
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
  const std::filesystem::path path = (argc > 2) ? argv[2] : current_path;

  if (command == "validate") {
    return handle_validate(path);
  }
  if (command == "generate") {
    return handle_generate(path);
  }
  if (command == "new") {
    return handle_new(argc, argv);
  }

  std::cerr << "Unknown command: " << command << std::endl;
  print_usage();
  return 2;
}
