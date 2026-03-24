#pragma once

#include "media_types.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace media_analyser {

class MediaDecoderError : public std::runtime_error {
public:
  explicit MediaDecoderError(std::string_view message);
};

class IMediaDecoder {
public:
  virtual MediaInfo decode(ByteSpan buffer) = 0;
  virtual ~IMediaDecoder() = default;
};

class Mp3MediaDecoder : public IMediaDecoder {
public:
  MediaInfo decode(ByteSpan buffer) override;

private:
  void prepare_buffer(ByteSpan& buffer) const;
  std::optional<MediaInfo> parse_xing_header(ByteSpan& buffer,
                                             int sample_rate) const;
  MediaInfo scan_frames(const std::byte* begin, const std::byte* end,
                        int sample_rate) const;

  bool is_mpeg1_l3_header(const std::byte* hdr) const;
  void trim_id3_tags(ByteSpan& buffer) const;

  int get_bitrate(const std::byte* hdr) const;
  int get_sample_rate(const std::byte* hdr) const;
  std::size_t get_frame_size(const std::byte* hdr) const;
  std::size_t get_xing_tag_offset(const std::byte* hdr) const;

  // ── MPEG-1 Layer III bitrates (kbps)
  // Index 0 = free-format (unsupported), index 15 = reserved (invalid).
  static constexpr std::array<int, 16> BITRATES{
      0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};

  // ── MPEG-1 Layer III sample rates (Hz)
  // Index 3 = reserved (invalid).
  static constexpr std::array<int, 4> SAMPLE_RATES = {44100, 48000, 32000, -1};
};

} // namespace media_analyser
