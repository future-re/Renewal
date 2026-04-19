#pragma once
#include <string_view>

namespace renewal {
namespace templates {

inline constexpr std::string_view pkg_toml = R"(name = "{{name}}"
version = "0.1.0"
description = "A Renewal package {{name}}"
)";

inline constexpr std::string_view mod_cppm = R"(export module {{name}};

// module implementation for {{name}}
)";

inline constexpr std::string_view hello_cpp = R"(#include <iostream>

void {{name}}::hello() {
    std::cout << "Hello from Renewal module {{name}}!" << std::endl;
}
)";

inline constexpr std::string_view main_cpp = R"(#include <iostream>

int main() {
    std::cout << "Hello from tests for {{name}}" << std::endl;
    return 0;
}
)";

}  // namespace templates
}  // namespace renewal
