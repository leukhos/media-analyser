#include "media_analyser.hpp"

#include "file_loader.hpp"
#include "logger.hpp"
#include "media_decoder.hpp"
#include "media_types.hpp"

#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace media_analyser {

MediaAnalyser::MediaAnalyser(std::shared_ptr<IMediaDecoder> decoder)
    : m_decoder{std::move(decoder)},
      m_file_loader{std::make_shared<IfstreamFileLoader>()},
      m_logger{std::make_shared<StdErrLogger>()} {}

bool MediaAnalyser::is_supported(const std::string& filename) const {
  return false;
}

MediaInfo MediaAnalyser::analyse(const std::string& filename) const {
  return {};
}

} // namespace media_analyser
