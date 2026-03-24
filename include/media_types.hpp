#pragma once

#include <cstddef>
#include <cstdint>

namespace media_analyser {

struct ByteSpan {
  const std::byte* data;
  std::size_t size;
};

struct MediaInfo {
  bool is_valid = false;
  std::uint32_t frame_count = 0;
  int sample_rate = 0;     // Hz
  float average_bitrate = 0.0f; // kbps
};

} // namespace media_analyser
