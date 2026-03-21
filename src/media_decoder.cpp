#include "media_decoder.hpp"
#include "media_types.hpp"

namespace media_analyser {

bool Mp3MediaDecoder::is_supported(ByteSpan buffer) {
  return false;
}

MediaInfo Mp3MediaDecoder::decode(ByteSpan buffer) {
  return MediaInfo();
}

} // namespace media_analyser
