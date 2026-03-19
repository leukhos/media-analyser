#pragma once

#include <cstddef>

namespace media_analyser {

struct ByteSpan {
  const std::byte* data;
  std::size_t size;
};

struct MediaInfo {
  bool is_valid = false;
  int frame_count = 0;
  int sample_rate = 0;     // Hz
  int average_bitrate = 0; // kbps
};

} // namespace media_analyser
