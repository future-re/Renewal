#include "renewal/create_new.hpp"
#include <expected>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "renewal/templates.hpp"
#include "renewal/utils.hpp"

namespace renewal {

namespace fs = std::filesystem;

static std::string render_template(std::string_view tmpl,
                                   const std::string& name) {
  return utils::replace_all(std::string(tmpl), "{{name}}", name);
}

std::expected<void, std::string> create_new(const std::string& name) {
  const fs::path base = name;

  if (fs::exists(base)) {
    throw std::runtime_error("Directory already exists: " + base.string());
  }

  fs::create_directories(base / "modules" / name);
  fs::create_directories(base / "tests");

  try {
    // 渲染并写入模板
    utils::write_file(base / "pkg.toml",
                      render_template(templates::pkg_toml, name));
    utils::write_file(base / "modules" / name / "mod.cppm",
                      render_template(templates::mod_cppm, name));
    utils::write_file(base / "modules" / name / "hello.cpp",
                      render_template(templates::hello_cpp, name));
    utils::write_file(base / "tests" / "main.cpp",
                      render_template(templates::main_cpp, name));

    std::cout << "Created Renewal package: " << base.string() << std::endl;
    std::cout << "  - " << (base / "pkg.toml").string() << std::endl;
    std::cout << "  - " << (base / "modules" / name / "mod.cppm").string()
              << std::endl;
    std::cout << "  - " << (base / "modules" / name / "hello.cpp").string()
              << std::endl;
    std::cout << "  - " << (base / "tests" / "main.cpp").string() << std::endl;
  } catch (const std::exception& e) {
    return std::unexpected("Error: " + std::string(e.what()));
  }

  return {};
}

}  // namespace renewal
