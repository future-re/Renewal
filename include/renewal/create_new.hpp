#pragma once
#include <expected>
#include <string>

namespace renewal {
std::expected<void, std::string> create_new(const std::string& name);
}
