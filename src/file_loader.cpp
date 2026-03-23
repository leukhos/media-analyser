#include "file_loader.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace media_analyser {

FileLoaderError::FileLoaderError(std::string_view message)
    : std::runtime_error{message.data()} {}

std::vector<std::byte>
IfstreamFileLoader::load(const std::filesystem::path& path) {
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  if (!file) {
    throw FileLoaderError{"Failed to open file: " + path.string()};
  }

  const auto size = static_cast<std::streamsize>(file.tellg());
  file.seekg(0);

  std::vector<std::byte> buffer(static_cast<std::size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), size);
  if (file.gcount() != size) {
    throw FileLoaderError{"Failed to read file: " + path.string()};
  }

  return buffer;
}

} // namespace media_analyser
