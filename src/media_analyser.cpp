#include "media_analyser.hpp"
#include "file_loader.hpp"
#include "logger.hpp"
#include "media_decoder.hpp"
#include "media_types.hpp"

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

MediaInfo MediaAnalyser::analyse(const std::filesystem::path& path) const {
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
