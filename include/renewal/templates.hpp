#pragma once
#include <string_view>

namespace renewal {
namespace templates {

inline constexpr std::string_view pkg_toml = R"([package]
name = "{{name}}"
version = "0.1.0"
edition = "c++23"
description = "A Renewal package {{name}}"

[deps]

[build]
target = "pkg_{{name}}"
type = "module_library"
sources = [
  "modules/{{name}}/hello.cpp"
]

[metadata]
)";

inline constexpr std::string_view mod_cppm = R"(export module {{name}};

export namespace {{name}} {
const char* hello();
}
)";

inline constexpr std::string_view hello_cpp = R"(module {{name}};

namespace {{name}} {

const char* hello() {
  return "Hello from Renewal module {{name}}!";
}

}
)";

inline constexpr std::string_view main_cpp = R"(import {{name}};

int main() {
  return {{name}}::hello()[0] == '\0';
}
)";

}  // namespace templates
}  // namespace renewal
