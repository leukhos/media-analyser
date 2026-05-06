#pragma once

#include <string_view>

namespace media_analyser {

enum class LogLevel { Debug = 0, Info, Warning, Error };

class ILogger {
public:
  virtual void log(LogLevel level, std::string_view message) = 0;
  virtual ~ILogger() = default;
};

class StdErrLogger : public ILogger {
public:
  void log(LogLevel level, std::string_view message) override;
};

} // namespace media_analyser
