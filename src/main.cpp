#include "renewal/create_new.hpp"
#include "renewal/validator.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void print_usage() {
  std::cerr << "Usage: renewal <command>" << std::endl;
  std::cerr << "Commands:" << std::endl;
  std::cerr << "  validate   Validate a package at the current path"
            << std::endl;
  std::cerr << "  new        Create a new package at the current path"
            << std::endl;
}

void handle_validate(const std::string& path) {
  auto result = renewal::validator(path);
  if (!result) {
    std::cerr << "Validation failed: " << result.error() << std::endl;
  } else {
    std::cout << "Validation passed" << std::endl;
  }
}

void handle_new(const std::string& path) {
  auto result = renewal::create_new(path);
  if (!result) {
    std::cerr << "Failed to create new package: " << result.error()
              << std::endl;
  } else {
    std::cout << "New package created at " << path << std::endl;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    print_usage();
    return 2;
  }

  auto current_path = std::filesystem::current_path();
  std::string command = argv[1];
  std::string path = (argc > 2) ? argv[2] : current_path.string();

  if (command == "validate") {
    handle_validate(path);
  } else if (command == "new") {
    handle_new(path);
  } else {
    std::cerr << "Unknown command: " << command << std::endl;
    print_usage();
    return 2;
  }

  return 0;
}