#include "renewal/create_new.hpp"

#include <expected>
#include <filesystem>
#include <string>

#include "renewal/templates.hpp"
#include "renewal/utils.hpp"

namespace renewal {

namespace fs = std::filesystem;

namespace {

std::string render_template(std::string_view tmpl, const std::string& name) {
  return utils::replace_all(std::string(tmpl), "{{name}}", name);
}

}  // namespace

std::expected<void, std::string> create_new(const fs::path& root_path) {
  if (root_path.empty()) {
    return std::unexpected("Package path must not be empty");
  }

  const fs::path package_root = root_path.lexically_normal();
  const std::string package_name = package_root.filename().string();
  if (package_name.empty()) {
    return std::unexpected("Package path must include a directory name");
  }

  if (fs::exists(package_root)) {
    return std::unexpected("Directory already exists: " +
                           package_root.string());
  }

  try {
    fs::create_directories(package_root / "modules" / package_name);
    fs::create_directories(package_root / "tests");

    utils::write_file(package_root / "pkg.toml",
                      render_template(templates::pkg_toml, package_name));
    utils::write_file(package_root / "modules" / package_name / "mod.cppm",
                      render_template(templates::mod_cppm, package_name));
    utils::write_file(package_root / "modules" / package_name / "hello.cpp",
                      render_template(templates::hello_cpp, package_name));
    utils::write_file(package_root / "tests" / "main.cpp",
                      render_template(templates::main_cpp, package_name));
  } catch (const std::exception& error) {
    return std::unexpected(error.what());
  }

  return {};
}

}  // namespace renewal
