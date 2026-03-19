#include "logger.hpp"

#include <iostream>
#include <string_view>

namespace media_analyser {

constexpr std::string_view to_string(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARNING";
  case LogLevel::Error:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

void StdErrLogger::log(LogLevel level, std::string_view message) {
  std::cerr << "[" << to_string(level) << "] " << message << '\n';
}

} // namespace media_analyser
