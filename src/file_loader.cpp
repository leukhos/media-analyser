#include "file_loader.hpp"

#include <fstream>
#include <stdexcept>

namespace media_analyser {

std::vector<std::byte>
IfstreamFileLoader::load(const std::filesystem::path& path) {
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  if (!file) {
    throw std::runtime_error{"Failed to open file: " + path.string()};
  }

  const auto size = static_cast<std::size_t>(file.tellg());
  file.seekg(0);

  std::vector<std::byte> buffer(size);
  file.read(reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(size));
  if (!file) {
    throw std::runtime_error{"Failed to read file: " + path.string()};
  }

  return buffer;
}

} // namespace media_analyser
