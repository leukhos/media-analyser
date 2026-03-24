#include "media_decoder.hpp"
#include "media_types.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace media_analyser {

MediaDecoderError::MediaDecoderError(std::string_view message)
    : std::runtime_error{message.data()} {}

MediaInfo Mp3MediaDecoder::decode(ByteSpan buffer) {
  prepare_buffer(buffer);

  const int sample_rate = get_sample_rate(buffer.data);
  auto result = parse_xing_header(buffer, sample_rate);

  return result.value_or(
      scan_frames(buffer.data, buffer.data + buffer.size, sample_rate));
}

void Mp3MediaDecoder::prepare_buffer(ByteSpan& buffer) const {
  if (buffer.size < 4)
    throw MediaDecoderError("buffer too small to contain an MP3 frame");

  trim_id3_tags(buffer);

  if (buffer.size < 4)
    throw MediaDecoderError("no audio data found after stripping ID3 tags");

  const std::byte* end = buffer.data + buffer.size;
  const std::byte* hdr = buffer.data;

  // Advance past any leading bytes that don't form an MP3 sync word.
  while (hdr + 4 <= end && !(hdr[0] == std::byte{0xFF} &&
                              (hdr[1] & std::byte{0xE0}) == std::byte{0xE0}))
    ++hdr;

  if (hdr + 4 > end)
    throw MediaDecoderError("not an MP3 file: no sync word found");

  buffer.data = hdr;
  buffer.size = static_cast<std::size_t>(end - hdr);

  if ((hdr[1] & std::byte{0xFE}) != std::byte{0xFA})
    throw MediaDecoderError(
        "unsupported format: only MPEG-1 Layer III is supported");
  const auto br_idx = hdr[2] >> 4;
  if (br_idx == std::byte{0})
    throw MediaDecoderError("free-format bitrate is not supported");
  if (br_idx == std::byte{0x0F})
    throw MediaDecoderError("invalid frame: reserved bitrate index");
  if ((hdr[2] >> 2 & std::byte{0x3}) == std::byte{0x3})
    throw MediaDecoderError("invalid frame: reserved sample rate index");
}

// The Xing/Info tag sits at a fixed offset inside the first frame.
// When present, use its embedded frame count and byte count for a fast
// return; the tag frame itself contains no audio and must be skipped.
std::optional<MediaInfo>
Mp3MediaDecoder::parse_xing_header(ByteSpan& buffer, int sample_rate) const {
  const std::byte* hdr = buffer.data;
  const std::size_t xoff = get_xing_tag_offset(hdr);
  std::size_t frame_size = get_frame_size(hdr);

  if (xoff + 8 > frame_size)
    return std::nullopt;

  const std::byte* tag = hdr + xoff;
  if (std::memcmp(tag, "Xing", 4) != 0 && std::memcmp(tag, "Info", 4) != 0)
    return std::nullopt;

  buffer.data += frame_size; // skip the Xing tag frame; it has no audio data

  const std::uint32_t flags = (std::to_integer<std::uint32_t>(tag[4]) << 24) |
                              (std::to_integer<std::uint32_t>(tag[5]) << 16) |
                              (std::to_integer<std::uint32_t>(tag[6]) << 8) |
                              std::to_integer<std::uint32_t>(tag[7]);

  // Both FRAMES_FLAG (0x1) and BYTES_FLAG (0x2) must be present.
  if ((flags & 0x3) != 0x3 || xoff + 16 > frame_size)
    return std::nullopt;

  const std::uint32_t total_frames =
      (std::to_integer<std::uint32_t>(tag[8]) << 24) |
      (std::to_integer<std::uint32_t>(tag[9]) << 16) |
      (std::to_integer<std::uint32_t>(tag[10]) << 8) |
      std::to_integer<std::uint32_t>(tag[11]);
  const std::uint32_t total_bytes =
      (std::to_integer<std::uint32_t>(tag[12]) << 24) |
      (std::to_integer<std::uint32_t>(tag[13]) << 16) |
      (std::to_integer<std::uint32_t>(tag[14]) << 8) |
      std::to_integer<std::uint32_t>(tag[15]);

  if (total_frames == 0 || total_bytes <= frame_size)
    return std::nullopt;

  // Xing/Info header frame is included in total_bytes but not in
  // total_frames, so subtract it before computing the average bitrate.
  // avg_bitrate (kbps) = audio_bytes * 8 / duration / 1000
  //   audio_bytes  = total_bytes - frame_size (Xing frame)
  //   duration (s) = total_frames * 1152 / sample_rate
  const float duration = total_frames * 1152.0f / sample_rate;
  const float avg_br = (total_bytes - frame_size) * 8.0f / duration / 1000.0f;
  return MediaInfo{true, total_frames, sample_rate, avg_br};
}

// Walk every audio frame, accumulating frame count and total bitrate.
MediaInfo Mp3MediaDecoder::scan_frames(const std::byte* begin,
                                       const std::byte* end,
                                       int sample_rate) const {
  std::uint32_t frame_count = 0;
  std::uint64_t total_br = 0;

  while (begin < end && is_mpeg1_l3_header(begin)) {
    total_br += get_bitrate(begin);
    begin += get_frame_size(begin);
    ++frame_count;
  }

  const float avg_br =
      frame_count > 0 ? static_cast<float>(total_br) / frame_count : 0.0f;
  return MediaInfo{frame_count > 0, frame_count, sample_rate, avg_br};
}

bool Mp3MediaDecoder::is_mpeg1_l3_header(const std::byte* hdr) const {
  // Byte 0: sync word (0xFF).
  // Byte 1 masked 0xFE: sync(111) + MPEG1(11) + LayerIII(01); CRC bit ignored.
  if (hdr[0] != std::byte{0xFF} ||
      (hdr[1] & std::byte{0xFE}) != std::byte{0xFA})
    return false;
  // Byte 2 bits 7-4: bitrate index. 0 = free-format, 15 = reserved.
  const auto br_idx = hdr[2] >> 4;
  if (br_idx == std::byte{0} || br_idx == std::byte{0x0F})
    return false;
  // Byte 2 bits 3-2: sample rate index. 3 = reserved.
  return (hdr[2] >> 2 & std::byte{0x3}) != std::byte{0x3};
}

// Strips leading ID3v2 and trailing ID3v1 tags, returning a ByteSpan over
// the remaining audio data. Throws if the ID3v2 tag spans the entire buffer.
void Mp3MediaDecoder::trim_id3_tags(ByteSpan& buffer) const {
  const std::byte* data = buffer.data;
  std::size_t size = buffer.size;

  // Skip leading ID3v2 tag ("ID3" marker + 7-byte header = 10 bytes minimum).
  // Bytes 6-9 encode the tag size as a syncsafe integer; byte 5 bit 4 signals
  // a footer (+10 bytes).
  if (size >= 10 && std::memcmp(data, "ID3", 3) == 0) {

    std::size_t tag_size =
        (std::to_integer<std::size_t>(data[6] & std::byte{0x7F}) << 21) |
        (std::to_integer<std::size_t>(data[7] & std::byte{0x7F}) << 14) |
        (std::to_integer<std::size_t>(data[8] & std::byte{0x7F}) << 7) |
        std::to_integer<std::size_t>(data[9] & std::byte{0x7F});
    tag_size += 10; // 10-bytes header
    if ((data[5] & std::byte{0x10}) != std::byte{0})
      tag_size += 10; // footer present
    if (tag_size >= size)
      throw MediaDecoderError("ID3v2 tag spans the entire buffer");

    data += tag_size;
    size -= tag_size;
  }

  // Trim trailing ID3v1 tag (fixed 128-byte block starting with "TAG").
  if (size >= 128 && std::memcmp(data + size - 128, "TAG", 3) == 0)
    size -= 128;

  buffer.data = data;
  buffer.size = size;
}

int Mp3MediaDecoder::get_bitrate(const std::byte* hdr) const {
  return BITRATES[std::to_integer<std::size_t>(hdr[2] >> 4)];
}

int Mp3MediaDecoder::get_sample_rate(const std::byte* hdr) const {
  return SAMPLE_RATES[std::to_integer<std::size_t>((hdr[2] >> 2) &
                                                   std::byte{3})];
}

std::size_t Mp3MediaDecoder::get_frame_size(const std::byte* hdr) const {
  std::size_t frame_size = 1152 * get_bitrate(hdr) * 125 / get_sample_rate(hdr);
  std::size_t padding =
      std::to_integer<std::size_t>((hdr[2] >> 1) & std::byte(1));

  return frame_size + padding;
}

// Byte offset within a frame where the Xing/Info tag begins.
// Comes right after: header (4) + optional CRC (2) + side info (17 mono/32
// stereo).
std::size_t Mp3MediaDecoder::get_xing_tag_offset(const std::byte* hdr) const {
  const bool has_crc = (hdr[1] & std::byte{0x01}) == std::byte{0};
  const bool is_mono = (hdr[3] & std::byte{0xC0}) == std::byte{0xC0};
  return 4ul + (has_crc ? 2ul : 0ul) + (is_mono ? 17ul : 32ul);
}

} // namespace media_analyser
