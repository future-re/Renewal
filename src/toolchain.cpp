#include "renewal/toolchain.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <optional>
#include <re2/re2.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace renewal {

namespace {

#ifdef _WIN32
FILE* open_pipe(const char* command, const char* mode) {
  return _popen(command, mode);
}

int close_pipe(FILE* pipe) {
  return _pclose(pipe);
}
#else
FILE* open_pipe(const char* command, const char* mode) {
  return popen(command, mode);
}

int close_pipe(FILE* pipe) {
  return pclose(pipe);
}
#endif

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

std::optional<std::string> getenv_string(const char* key) {
  const char* value = std::getenv(key);
  if (!value || *value == '\0') {
    return std::nullopt;
  }
  return std::string(value);
}

bool command_exists(const std::string& command) {
#ifdef _WIN32
  const std::string probe = "where " + shell_quote(command) + " >nul 2>&1";
#else
  const std::string probe =
      "command -v " + shell_quote(command) + " >/dev/null 2>&1";
#endif
  return std::system(probe.c_str()) == 0;
}

std::expected<std::string, std::string> capture_command(
    const std::string& command) {
  FILE* pipe = open_pipe(command.c_str(), "r");
  if (!pipe) {
    return std::unexpected("Failed to run command: " + command);
  }

  std::array<char, 4096> buffer{};
  std::string output;
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr) {
    output += buffer.data();
  }

  if (close_pipe(pipe) != 0) {
    return std::unexpected("Command failed: " + command);
  }

  return output;
}

std::optional<int> first_version_component(const std::string& text) {
  std::string version;
  if (!RE2::PartialMatch(text, R"((\d+)(?:\.\d+)*)", &version)) {
    return std::nullopt;
  }
  return std::stoi(version);
}

std::expected<ToolchainInfo, std::string> describe_compiler(
    const std::string& executable) {
  auto version_output =
      capture_command(shell_quote(executable) + " --version 2>&1");
  if (!version_output) {
    version_output = capture_command(shell_quote(executable) + " /? 2>&1");
    if (!version_output) {
      return std::unexpected("Failed to inspect compiler: " + executable);
    }
  }

  ToolchainInfo info;
  info.executable = executable;
  info.version_text = *version_output;

  const std::string_view view(info.version_text);
  if (view.find("clang") != std::string_view::npos) {
    info.family = ToolchainFamily::Clang;
    info.major_version = first_version_component(info.version_text);
    info.preferred_generator = "Ninja";
  } else if (view.find("gcc") != std::string_view::npos ||
             view.find("g++") != std::string_view::npos ||
             view.find("Free Software Foundation") != std::string_view::npos) {
    info.family = ToolchainFamily::Gcc;
    info.major_version = first_version_component(info.version_text);
    info.preferred_generator = "Ninja";
  } else if (view.find("Microsoft") != std::string_view::npos ||
             view.find("MSVC") != std::string_view::npos) {
    info.family = ToolchainFamily::Msvc;
    info.major_version = first_version_component(info.version_text);
  } else {
    info.family = ToolchainFamily::Unknown;
  }

  return info;
}

std::vector<std::string> compiler_candidates() {
  std::vector<std::string> candidates;

  if (auto compiler = getenv_string("RENEWAL_CXX_COMPILER")) {
    candidates.push_back(*compiler);
  }
  if (auto compiler = getenv_string("CMAKE_CXX_COMPILER")) {
    candidates.push_back(*compiler);
  }
  if (auto compiler = getenv_string("CXX")) {
    candidates.push_back(*compiler);
  }

  candidates.push_back("clang++");
  candidates.push_back("clang++-20");
  candidates.push_back("clang++-19");
  candidates.push_back("clang++-18");
  candidates.push_back("clang++-17");
  candidates.push_back("clang++-16");
  candidates.push_back("g++");
  candidates.push_back("c++");
  candidates.push_back("cl");
  return candidates;
}

std::expected<ToolchainInfo, std::string> validate_toolchain(
    ToolchainInfo info) {
  switch (info.family) {
    case ToolchainFamily::Clang:
      if (!info.major_version || *info.major_version < 16) {
        return std::unexpected(
            "Clang 16 or newer is required for CMake C++ modules support");
      }
      info.configure_args.push_back("-DCMAKE_CXX_COMPILER=" + info.executable);
      info.configure_args.push_back("-DCMAKE_CXX_SCAN_FOR_MODULES=ON");
      return info;

    case ToolchainFamily::Gcc:
      if (!info.major_version || *info.major_version < 14) {
        return std::unexpected(
            "GCC 14 or newer is required for CMake C++ modules support");
      }
      info.configure_args.push_back("-DCMAKE_CXX_COMPILER=" + info.executable);
      info.configure_args.push_back("-DCMAKE_CXX_SCAN_FOR_MODULES=ON");
      return info;

    case ToolchainFamily::Msvc:
      if (!info.major_version || *info.major_version < 19) {
        return std::unexpected(
            "MSVC 19.34 / VS 2022 17.4 or newer is required for CMake C++ "
            "modules support");
      }
      info.configure_args.push_back("-DCMAKE_CXX_SCAN_FOR_MODULES=ON");
      return info;

    case ToolchainFamily::Unknown:
      return std::unexpected(
          "Unable to classify the active C++ compiler for CMake modules "
          "support");
  }

  return std::unexpected("Unhandled toolchain family");
}

}  // namespace

std::expected<ToolchainInfo, std::string> detect_toolchain() {
  std::vector<std::string> attempted;

  for (const auto& candidate : compiler_candidates()) {
    if (candidate.empty()) {
      continue;
    }

    attempted.push_back(candidate);
    if (!command_exists(candidate)) {
      continue;
    }

    auto described = describe_compiler(candidate);
    if (!described) {
      continue;
    }

    auto validated = validate_toolchain(*described);
    if (validated) {
      return validated;
    }

    if (candidate == attempted.front()) {
      return std::unexpected(validated.error());
    }
  }

  std::ostringstream message;
  message << "No supported C++ toolchain found. Tried:";
  for (const auto& candidate : attempted) {
    message << " " << candidate;
  }
  return std::unexpected(message.str());
}

std::string toolchain_family_name(ToolchainFamily family) {
  switch (family) {
    case ToolchainFamily::Clang:
      return "clang";
    case ToolchainFamily::Gcc:
      return "gcc";
    case ToolchainFamily::Msvc:
      return "msvc";
    case ToolchainFamily::Unknown:
      return "unknown";
  }

  return "unknown";
}

std::string format_toolchain_report(const ToolchainInfo& toolchain) {
  std::ostringstream report;
  report << "Toolchain:\n";
  report << "  family: " << toolchain_family_name(toolchain.family) << "\n";
  report << "  compiler: " << toolchain.executable << "\n";
  if (toolchain.major_version) {
    report << "  major_version: " << *toolchain.major_version << "\n";
  }
  if (toolchain.preferred_generator) {
    report << "  preferred_generator: " << *toolchain.preferred_generator
           << "\n";
  }
  if (!toolchain.configure_args.empty()) {
    report << "  configure_args:\n";
    for (const auto& arg : toolchain.configure_args) {
      report << "    " << arg << "\n";
    }
  }
  return report.str();
}

}  // namespace renewal
