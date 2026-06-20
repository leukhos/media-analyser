#include "media_analyser.h"
#include "file_loader.h"
#include "logger.h"
#include "media_decoder.h"
#include "media_types.h"

#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace fs = std::filesystem;

namespace media_analyser {

MediaAnalyser::MediaAnalyser(std::shared_ptr<IMediaDecoder> decoder)
    : m_decoder{std::move(decoder)},
      m_file_loader{std::make_shared<IfstreamFileLoader>()},
      m_logger{std::make_shared<StdErrLogger>()} {}

MediaAnalyser::MediaAnalyser(std::shared_ptr<IMediaDecoder> decoder,
                             std::shared_ptr<IFileLoader> file_loader,
                             std::shared_ptr<ILogger> logger)
    : m_decoder{std::move(decoder)}, m_file_loader{std::move(file_loader)},
      m_logger{std::move(logger)} {}

MediaInfo MediaAnalyser::analyse(const fs::path& path) const {
  try {
    auto buf = m_file_loader->load(path);
    return m_decoder->decode({buf.data(), buf.size()});
  } catch (const FileLoaderError& e) {
    m_logger->log(LogLevel::Error, std::string("FileLoader: ") + e.what());
  } catch (const MediaDecoderError& e) {
    m_logger->log(LogLevel::Error, std::string("MediaDecoder: ") + e.what());
  } catch (const std::exception& e) {
    m_logger->log(LogLevel::Error, e.what());
  }

  return {};
}

} // namespace media_analyser
