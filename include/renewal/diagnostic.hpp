#pragma once

#include <filesystem>
#include <string>

namespace renewal {

enum class DiagnosticSeverity {
  Error,
  Warning,
};

struct Diagnostic {
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  std::string message;
  std::filesystem::path path;
};

}  // namespace renewal
