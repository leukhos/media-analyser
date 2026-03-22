#include "media_decoder.hpp"
#include "media_types.hpp"

#include <cstdint>
#include <cstring>

namespace media_analyser {

namespace {

// ── MPEG-1 Layer III bitrates (kbps), indexed by header bits [2]>>4 ──────────
// Index 0 = free-format (unsupported), index 15 = reserved (invalid).
constexpr int MPEG1_L3_BITRATES[16] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};

// ── MPEG-1 sample rates (Hz), indexed by header bits ([2]>>2)&3 ──────────────
// Index 3 = reserved (invalid).
constexpr int MPEG1_SAMPLE_RATES[4] = {44100, 48000, 32000, -1};

// Returns true if the 4 bytes at `h` carry a valid MPEG audio sync word
// (any version, any layer). Rejects reserved version (01) and reserved layer
// (00) but does NOT enforce MPEG-1 or Layer III specifically.
bool is_mpeg_sync_hdr(const uint8_t* h) noexcept {
  if (h[0] != 0xFF || (h[1] & 0xE0) != 0xE0)
    return false;
  if (((h[1] >> 3) & 0x3) == 0x1) // reserved MPEG version
    return false;
  if (((h[1] >> 1) & 0x3) == 0x0) // reserved layer
    return false;
  return true;
}

// Returns true if the 4 bytes at `h` form a valid MPEG-1 Layer III header.
// Byte 1 mask 0xFE: sync(111) + version(11=MPEG1) + layer(01=III), CRC ignored.
bool is_mpeg1_l3_hdr(const uint8_t* h) noexcept {
  if (h[0] != 0xFF || (h[1] & 0xFE) != 0xFA)
    return false;
  const int br = h[2] >> 4;
  if (br == 0 || br == 15)
    return false; // free-format or reserved bitrate
  if (((h[2] >> 2) & 3) == 3)
    return false; // reserved sample rate
  return true;
}

int hdr_bitrate_kbps(const uint8_t* h) noexcept {
  return MPEG1_L3_BITRATES[h[2] >> 4];
}

int hdr_sample_rate_hz(const uint8_t* h) noexcept {
  return MPEG1_SAMPLE_RATES[(h[2] >> 2) & 3];
}

// Frame size in bytes without padding: 1152 * bitrate * 125 / sample_rate
int hdr_frame_bytes(const uint8_t* h) noexcept {
  return 1152 * hdr_bitrate_kbps(h) * 125 / hdr_sample_rate_hz(h);
}

// Padding: 0 or 1 byte for Layer III
int hdr_frame_padding(const uint8_t* h) noexcept { return (h[2] >> 1) & 1; }

int hdr_frame_total(const uint8_t* h) noexcept {
  return hdr_frame_bytes(h) + hdr_frame_padding(h);
}

// Byte offset within a frame where the Xing/Info tag begins.
// Comes right after: header (4) + optional CRC (2) + side info (17 mono/32 stereo).
size_t xing_tag_offset(const uint8_t* h) noexcept {
  const bool has_crc = !(h[1] & 1);
  const bool is_mono = (h[3] & 0xC0) == 0xC0;
  return 4u + (has_crc ? 2u : 0u) + (is_mono ? 17u : 32u);
}

// ── ID3 stripping ─────────────────────────────────────────────────────────────

// Returns bytes to skip over a leading ID3v2 tag, or 0 if none present.
size_t skip_id3v2(const uint8_t* buf, size_t size) noexcept {
  if (size < 10 || memcmp(buf, "ID3", 3) != 0)
    return 0;
  if ((buf[5] & 15) || (buf[6] & 0x80) || (buf[7] & 0x80) || (buf[8] & 0x80) ||
      (buf[9] & 0x80))
    return 0;
  size_t tag_size =
      static_cast<size_t>(((buf[6] & 0x7F) << 21) | ((buf[7] & 0x7F) << 14) |
                           ((buf[8] & 0x7F) << 7) | (buf[9] & 0x7F)) +
      10;
  if (buf[5] & 0x10)
    tag_size += 10; // footer
  return tag_size;
}

// Returns reduced size after trimming a trailing ID3v1 tag, if present.
size_t trim_id3v1(const uint8_t* buf, size_t size) noexcept {
  if (size >= 128 && memcmp(buf + size - 128, "TAG", 3) == 0)
    return size - 128;
  return size;
}

// Returns offset of the first MPEG audio sync header (any version/layer)
// in [data, data+size), or `size` if none found.
size_t find_first_mpeg_frame(const uint8_t* data, size_t size) noexcept {
  for (size_t i = 0; i + 4 <= size; ++i) {
    if (is_mpeg_sync_hdr(data + i))
      return i;
  }
  return size;
}

// Returns offset of the first valid MPEG-1 L3 header in [data, data+size),
// or `size` if none found.
size_t find_first_frame(const uint8_t* data, size_t size) noexcept {
  for (size_t i = 0; i + 4 <= size; ++i) {
    if (is_mpeg1_l3_hdr(data + i))
      return i;
  }
  return size;
}

// ── Shared validation ─────────────────────────────────────────────────────────

// Returns nullptr on success, or a static error string on failure.
const char* validate(ByteSpan buffer) noexcept {
  if (buffer.size < 4)
    return "buffer too small to contain an MP3 frame";

  const auto* data = reinterpret_cast<const uint8_t*>(buffer.data);
  size_t size = buffer.size;

  const size_t id3v2_skip = skip_id3v2(data, size);
  if (id3v2_skip >= size)
    return "ID3v2 tag spans the entire buffer";
  data += id3v2_skip;
  size -= id3v2_skip;

  size = trim_id3v1(data, size);

  if (size < 4)
    return "no audio data found after stripping ID3 tags";

  const size_t pos = find_first_mpeg_frame(data, size);
  if (pos >= size)
    return "no MPEG audio frame found";

  // Byte 1, bits 4-3: MPEG version. 0b11 = MPEG 1 (only supported version).
  if (((data[pos + 1] >> 3) & 0x3) != 0x3)
    return "unsupported MPEG version (only MPEG 1 is supported)";

  // Byte 1, bits 2-1: layer. 0b01 = Layer III (only supported layer).
  if (((data[pos + 1] >> 1) & 0x3) != 0x1)
    return "unsupported MPEG layer (only Layer III is supported)";

  // Byte 2, bits 7-4: bitrate index. 0 = free-format, 15 = reserved.
  const int br = (data[pos + 2] >> 4) & 0xF;
  if (br == 0 || br == 15)
    return "free-format or reserved bitrate index in MPEG frame header";

  // Byte 2, bits 3-2: sample rate index. 3 = reserved.
  if (((data[pos + 2] >> 2) & 0x3) == 0x3)
    return "reserved sample rate index in MPEG frame header";

  return nullptr;
}

} // namespace

// ── Public interface ──────────────────────────────────────────────────────────

void Mp3MediaDecoder::check(ByteSpan buffer) const {
  const char* err = validate(buffer);
  if (err != nullptr)
    throw DecodeError(err);
}

MediaInfo Mp3MediaDecoder::decode(ByteSpan buffer) {
  check(buffer);

  const auto* data = reinterpret_cast<const uint8_t*>(buffer.data);
  size_t size = buffer.size;

  // Strip ID3 wrappers
  const size_t id3v2_skip = skip_id3v2(data, size);
  data += id3v2_skip;
  size -= id3v2_skip;
  size = trim_id3v1(data, size);

  // Locate the first audio (or Xing) frame
  const size_t pos = find_first_frame(data, size);
  const uint8_t* hdr = data + pos;
  const int sr = hdr_sample_rate_hz(hdr);
  const size_t first_total = static_cast<size_t>(hdr_frame_total(hdr));

  // ── Xing / Info header detection ─────────────────────────────────────────
  // The Xing/Info tag sits at a fixed offset inside the first frame.
  // When present, use its embedded frame count and byte count for a fast
  // return; the tag frame itself contains no audio and must be skipped.
  const size_t xoff = xing_tag_offset(hdr);
  bool is_xing_frame = false;

  if (xoff + 8 <= first_total && pos + xoff + 8 <= size) {
    const uint8_t* tag = hdr + xoff;
    if (memcmp(tag, "Xing", 4) == 0 || memcmp(tag, "Info", 4) == 0) {
      is_xing_frame = true;
      const uint32_t flags = (uint32_t{tag[4]} << 24) | (uint32_t{tag[5]} << 16) |
                             (uint32_t{tag[6]} << 8) | uint32_t{tag[7]};
      // Both FRAMES_FLAG (0x1) and BYTES_FLAG (0x2) must be present.
      if ((flags & 0x3) == 0x3 && xoff + 16 <= first_total &&
          pos + xoff + 16 <= size) {
        const uint8_t* p = tag + 8;
        const uint32_t total_frames = (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) |
                                      (uint32_t{p[2]} << 8) | uint32_t{p[3]};
        const uint32_t total_bytes  = (uint32_t{p[4]} << 24) | (uint32_t{p[5]} << 16) |
                                      (uint32_t{p[6]} << 8) | uint32_t{p[7]};
        if (total_frames > 0 && total_bytes > first_total) {
          // LAME includes the Xing/Info header frame in total_bytes but not in
          // total_frames, so subtract it before computing the average bitrate.
          // avg_bitrate (kbps) = audio_bytes * 8 / duration / 1000
          //   audio_bytes      = total_bytes - first_total (Xing frame size)
          //   duration (s)     = total_frames * 1152 / sample_rate
          const float duration =
              static_cast<float>(total_frames) * 1152.0f / static_cast<float>(sr);
          const int avg_br = static_cast<int>(
              static_cast<float>(total_bytes - first_total) * 8.0f / duration /
                  1000.0f +
              0.5f);
          return MediaInfo{true, static_cast<int>(total_frames), sr, avg_br};
        }
      }
    }
  }

  // ── Frame scan ───────────────────────────────────────────────────────────
  // Walk every audio frame, skipping the Xing/Info header frame if present.
  // All frames are counted, including flush frame at EOF.
  size_t cur = pos + (is_xing_frame ? first_total : 0);
  int frame_count = 0;
  long long total_br = 0;

  while (cur + 4 <= size && is_mpeg1_l3_hdr(data + cur)) {
    const int br = hdr_bitrate_kbps(data + cur);
    if (br == 0)
      break; // free-format: unsupported
    ++frame_count;
    total_br += br;
    cur += static_cast<size_t>(hdr_frame_total(data + cur));
  }

  const int avg_br =
      frame_count > 0 ? static_cast<int>(total_br / frame_count) : 0;
  return MediaInfo{frame_count > 0, frame_count, sr, avg_br};
}

} // namespace media_analyser
