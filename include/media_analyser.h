#pragma once

#include "media_types.h"

#include <filesystem>
#include <memory>

namespace media_analyser {

class IFileLoader;
class ILogger;
class IMediaDecoder;

class MediaAnalyser {
public:
  explicit MediaAnalyser(std::shared_ptr<IMediaDecoder> decoder);
  explicit MediaAnalyser(std::shared_ptr<IMediaDecoder> decoder,
                         std::shared_ptr<IFileLoader> file_loader,
                         std::shared_ptr<ILogger> logger);

  [[nodiscard]] MediaInfo analyse(const std::filesystem::path& path) const;

private:
  std::shared_ptr<IMediaDecoder> m_decoder;
  std::shared_ptr<IFileLoader> m_file_loader;
  std::shared_ptr<ILogger> m_logger;
};

} // namespace media_analyser
