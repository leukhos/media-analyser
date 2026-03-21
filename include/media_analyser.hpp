#pragma once

#include "media_types.hpp"

#include <memory>
#include <string>

namespace media_analyser {

class IFileLoader;
class ILogger;
class IMediaDecoder;

class MediaAnalyser {
public:
  explicit MediaAnalyser(std::shared_ptr<IMediaDecoder> decoder);

#ifdef MEDIA_ANALYSER_TEST
  void set_logger(const std::shared_ptr<ILogger>& logger) { m_logger = logger; }
  void set_file_loader(const std::shared_ptr<IFileLoader>& file_loader) {
    m_file_loader = file_loader;
  }
#endif

  bool is_supported(const std::string& filename) const;
  MediaInfo analyse(const std::string& filename) const;

private:
  std::shared_ptr<IMediaDecoder> m_decoder;
  std::shared_ptr<IFileLoader> m_file_loader;
  std::shared_ptr<ILogger> m_logger;
};

} // namespace media_analyser
