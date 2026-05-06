#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace media_analyser {

class FileLoaderError : public std::runtime_error {
public:
  explicit FileLoaderError(std::string_view message);
};

class IFileLoader {
public:
  virtual std::vector<std::byte> load(const std::filesystem::path& path) = 0;
  virtual ~IFileLoader() = default;
};

class IfstreamFileLoader : public IFileLoader {
public:
  std::vector<std::byte> load(const std::filesystem::path& path) override;
};

} // namespace media_analyser
