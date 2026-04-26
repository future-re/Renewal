#pragma once

#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace renewal {

enum class ToolchainFamily {
  Unknown,
  Clang,
  Gcc,
  Msvc,
};

struct ToolchainInfo {
  ToolchainFamily family = ToolchainFamily::Unknown;
  std::string executable;
  std::string version_text;
  std::optional<int> major_version;
  std::optional<std::string> preferred_generator;
  std::vector<std::string> configure_args;
};

std::expected<ToolchainInfo, std::string> detect_toolchain();
std::string toolchain_family_name(ToolchainFamily family);
std::string format_toolchain_report(const ToolchainInfo& toolchain);

}  // namespace renewal
