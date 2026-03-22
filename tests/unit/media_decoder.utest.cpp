#include "media_decoder.hpp"
#include "media_types.hpp"

#include <doctest/doctest.h>
#include <lame/lame.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace ma = media_analyser;

// ── MP3 frame header byte layout ─────────────────────────────────────────────
// Byte 0 : 0xFF           (sync)
// Byte 1 : sync(3) | version(2) | layer(2) | protection(1)
//            version : 11=MPEG1, 10=MPEG2, 00=MPEG2.5, 01=reserved
//            layer   : 01=III,   10=II,    11=I,        00=reserved
// Byte 2 : bitrate_idx(4) | sample_rate_idx(2) | padding(1) | private(1)
//            MPEG1 Layer III bitrates (kbps):
//              0001=32 … 0111=96, 1001=128 … 1011=192 … 1110=320
//            MPEG1 sample rates (Hz): 00=44100, 01=48000, 10=32000
// Byte 3 : channel_mode(2) | mode_ext(2) | copyright(1) | original(1)
//            | emphasis(2)   —   channel_mode: 00=stereo, 11=mono

// ── Headers for invalid-format tests ─────────────────────────────────────────

// MPEG-1 Layer I (not supported) — Byte1=0xFF: 111|11|11|1
static constexpr std::array<std::byte, 4> MPEG1_L1_HDR{
    std::byte{0xFF}, std::byte{0xFF}, std::byte{0x90}, std::byte{0x00}};

// MPEG-1 Layer II (not supported) — Byte1=0xFD: 111|11|10|1
static constexpr std::array<std::byte, 4> MPEG1_L2_HDR{
    std::byte{0xFF}, std::byte{0xFD}, std::byte{0x90}, std::byte{0x00}};

// MPEG-2 Layer III (not supported) — Byte1=0xF3: 111|10|01|1
static constexpr std::array<std::byte, 4> MPEG2_L3_HDR{
    std::byte{0xFF}, std::byte{0xF3}, std::byte{0x90}, std::byte{0x00}};

// MPEG-1 Layer III, bad bitrate index (0xF = reserved) — Byte2=0xF0
static constexpr std::array<std::byte, 4> MPEG1_L3_BAD_BITRATE_HDR{
    std::byte{0xFF}, std::byte{0xFB}, std::byte{0xF0}, std::byte{0x00}};

// MPEG-1 Layer III, reserved sample rate index (11) — Byte2=0x9C
static constexpr std::array<std::byte, 4> MPEG1_L3_BAD_SAMPLERATE_HDR{
    std::byte{0xFF}, std::byte{0xFB}, std::byte{0x9C}, std::byte{0x00}};

// ── Helpers
// ───────────────────────────────────────────────────────────────────

static ma::ByteSpan to_span(const std::vector<std::byte>& v) {
  return {v.data(), v.size()};
}

// Builds a zero-filled buffer of `size` bytes with the given 4-byte header.
// Only used for invalid-format inputs where the frame size is irrelevant.
static std::vector<std::byte>
make_invalid_frame(const std::array<std::byte, 4>& hdr, int size = 128) {
  std::vector<std::byte> buf(static_cast<size_t>(size), std::byte{0x00});
  std::copy(hdr.begin(), hdr.end(), buf.begin());
  return buf;
}

// ── lame-based MP3 generation
// ─────────────────────────────────────────────────

enum class Tags : unsigned {
  NONE = 0,
  HEADERED = 1 << 0, // Info tag (CBR) or Xing tag (VBR)
  ID3V1 = 1 << 1,
  ID3V2 = 1 << 2,
  VBR = 1 << 3,
};

inline Tags operator|(Tags a, Tags b) {
  return static_cast<Tags>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

inline bool operator&(Tags a, Tags b) {
  return static_cast<unsigned>(a) & static_cast<unsigned>(b);
}

struct Lame {
  lame_t gfp;
  Lame() : gfp(lame_init()) {}
  ~Lame() { lame_close(gfp); }
  Lame(const Lame&) = delete;
  Lame& operator=(const Lame&) = delete;
};

// Encodes `n_frames` of PCM silence with an already-configured lame handle
// and returns the resulting MP3 bytes.
// If a Xing/Info header was requested (bWriteVbrTag=1), lame writes a
// placeholder frame first; lame_get_lametag_frame() then returns the
// finalised tag which we overwrite in-place.  When an ID3v2 tag precedes
// the audio, lame_get_id3v2_tag() locates the correct write position.
static std::vector<std::byte> lame_encode_silence(lame_t gfp, int n_frames) {
  const int channels = lame_get_num_channels(gfp);
  const int n_pcm = 1152 * n_frames;
  std::vector<short> pcm(static_cast<size_t>(n_pcm * channels), 0);

  const int mp3_max = n_pcm + 7200;
  std::vector<unsigned char> mp3(static_cast<size_t>(mp3_max));

  int written = lame_encode_buffer_interleaved(gfp, pcm.data(), n_pcm,
                                               mp3.data(), mp3_max);
  int flushed = lame_encode_flush(gfp, mp3.data() + written, mp3_max - written);
  const int total = written + flushed;

  // Overwrite the Xing/Info placeholder with the finalised statistics.
  // The placeholder sits after any ID3v2 tag; lame_get_id3v2_tag() gives the
  // exact tag size so we know the precise write offset without any scanning.
  unsigned char tag[2048];
  const size_t tag_n = lame_get_lametag_frame(gfp, tag, sizeof(tag));
  if (tag_n > 0) {
    const size_t id3v2_size = lame_get_id3v2_tag(gfp, nullptr, 0);
    if (id3v2_size + tag_n <= static_cast<size_t>(total))
      std::copy(tag, tag + tag_n, mp3.begin() + id3v2_size);
  }

  std::vector<std::byte> result(static_cast<size_t>(total));
  std::transform(mp3.begin(), mp3.begin() + total, result.begin(),
                 [](unsigned char c) { return std::byte{c}; });
  return result;
}

// Generate an MP3 buffer using lame.
// Tags::VBR      : use variable bitrate (bitrate_kbps is ignored).
// Tags::HEADERED : prepend an Info/Xing header frame.
// Tags::ID3V1    : append an ID3v1 tag at the end of the stream.
// Tags::ID3V2    : prepend an ID3v2 tag at the start of the stream.
static std::vector<std::byte> make_mp3(int bitrate_kbps = 0,
                                       int sample_rate_hz = 44100,
                                       int channels = 2, Tags tags = Tags::NONE,
                                       int n_frames = 20) {
  Lame enc;
  lame_set_num_channels(enc.gfp, channels);
  lame_set_in_samplerate(enc.gfp, sample_rate_hz);
  lame_set_out_samplerate(enc.gfp, sample_rate_hz);
  if (tags & Tags::VBR) {
    lame_set_VBR(enc.gfp, vbr_default);
    lame_set_VBR_quality(enc.gfp, 4.0f);
  } else {
    lame_set_brate(enc.gfp, bitrate_kbps);
    lame_set_VBR(enc.gfp, vbr_off);
  }
  lame_set_quality(enc.gfp, 9);
  lame_set_bWriteVbrTag(enc.gfp, (tags & Tags::HEADERED) ? 1 : 0);
  if (tags & Tags::ID3V1 || tags & Tags::ID3V2) {
    id3tag_init(enc.gfp);
    id3tag_set_title(enc.gfp, "Test Track");
    if ((tags & Tags::ID3V1) && (tags & Tags::ID3V2))
      id3tag_add_v2(enc.gfp); // v1 is default after init, add_v2 prepends v2
    else if (tags & Tags::ID3V1)
      id3tag_v1_only(enc.gfp);
    else
      id3tag_v2_only(enc.gfp);
  }
  lame_init_params(enc.gfp);
  return lame_encode_silence(enc.gfp, n_frames);
}

// ── Tests
// ─────────────────────────────────────────────────────────────────────

TEST_SUITE_BEGIN("unit");

// ── decode
// ────────────────────────────────────────────────────────────────────

TEST_CASE("Mp3MediaDecoder::decode") {
  ma::Mp3MediaDecoder decoder;

  SUBCASE("invalid input throws DecodeError") {
    SUBCASE("empty buffer") {
      CHECK_THROWS_AS(decoder.decode(to_span({})), ma::DecodeError);
    }

    SUBCASE("1-byte buffer") {
      const std::vector<std::byte> buf{std::byte{0xFF}};

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("2-byte buffer") {
      const std::vector<std::byte> buf{std::byte{0xFF}, std::byte{0xFB}};

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("not an MP3 file — random non-sync bytes") {
      // 0x49 0x44 0x33 = "ID3" without a preceding valid MP3 frame
      const std::vector<std::byte> buf{std::byte{0x49}, std::byte{0x44},
                                       std::byte{0x33}, std::byte{0x03}};

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("not an MP3 file — all zeros") {
      const std::vector<std::byte> buf(128, std::byte{0x00});

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("MPEG-2 Layer III") {
      const auto buf = make_invalid_frame(MPEG2_L3_HDR);

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("MPEG-1 Layer I") {
      const auto buf = make_invalid_frame(MPEG1_L1_HDR);

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("MPEG-1 Layer II") {
      const auto buf = make_invalid_frame(MPEG1_L2_HDR);

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("bad bitrate index (0xF = reserved)") {
      const auto buf = make_invalid_frame(MPEG1_L3_BAD_BITRATE_HDR);

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }

    SUBCASE("reserved sample rate index (11)") {
      const auto buf = make_invalid_frame(MPEG1_L3_BAD_SAMPLERATE_HDR);

      CHECK_THROWS_AS(decoder.decode(to_span(buf)), ma::DecodeError);
    }
  }

  SUBCASE("Non-Headered") {
    // No Xing / VBRI info frame — the decoder must walk every audio frame.

    SUBCASE("CBR") {
      SUBCASE("single frame, 128 kbps, 44100 Hz") {
        const auto buf = make_mp3(128, 44100, 2, Tags::NONE, 1);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 2); // 1 requested frame + 1 LAME flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate == 128);
      }

      SUBCASE("multiple frames, 128 kbps, 44100 Hz") {
        const auto buf = make_mp3(128, 44100, 2, Tags::NONE, 10);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 11); // 10 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate == 128);
      }

      SUBCASE("48000 Hz — sample rate reported correctly") {
        const auto buf = make_mp3(128, 48000);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.sample_rate == 48000);
      }

      SUBCASE("320 kbps — bitrate reported correctly") {
        const auto buf = make_mp3(320);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.average_bitrate == 320);
      }

      SUBCASE("mono — side-info is 17 bytes instead of 32") {
        const auto buf = make_mp3(128, 44100, 1, Tags::NONE, 5);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 6); // 5 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate == 128);
      }
    }

    SUBCASE("VBR") {
      SUBCASE("stereo") {
        const auto buf = make_mp3(0, 44100, 2, Tags::VBR, 20);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 21); // 20 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate > 0);
      }

      SUBCASE("mono") {
        const auto buf = make_mp3(0, 44100, 1, Tags::VBR, 20);
        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 21); // 20 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate > 0);
      }
    }
  }

  SUBCASE("Headered") {
    // A Xing/Info header frame carries the real frame count and total byte
    // count so the decoder does not need to scan every frame.

    SUBCASE("CBR") {
      SUBCASE("Info tag, stereo") {
        const auto buf = make_mp3(128, 44100, 2, Tags::HEADERED, 10);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 11); // 10 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate == 128);
      }

      SUBCASE("Info tag, mono") {
        const auto buf = make_mp3(128, 44100, 1, Tags::HEADERED, 10);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 11); // 10 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate == 128);
      }
    }

    SUBCASE("VBR") {
      SUBCASE("Xing tag, stereo") {
        const auto buf = make_mp3(0, 44100, 2, Tags::VBR | Tags::HEADERED, 20);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 21); // 20 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate > 0);
      }

      SUBCASE("Xing tag, mono") {
        const auto buf = make_mp3(0, 44100, 1, Tags::VBR | Tags::HEADERED, 10);

        const ma::MediaInfo info = decoder.decode(to_span(buf));

        CHECK(info.is_valid);
        CHECK(info.frame_count == 11); // 10 requested frames + 1 flush frame
        CHECK(info.sample_rate == 44100);
        CHECK(info.average_bitrate > 0);
      }
    }
  }

  SUBCASE("ID3 metadata") {
    SUBCASE("ID3v2 at start of buffer") {
      const auto buf = make_mp3(128, 44100, 2, Tags::ID3V2, 5);

      const ma::MediaInfo info = decoder.decode(to_span(buf));

      CHECK(info.is_valid);
      CHECK(info.frame_count == 6); // 5 requested frames + 1 flush frame
      CHECK(info.sample_rate == 44100);
      CHECK(info.average_bitrate == 128);
    }

    SUBCASE("ID3v1 at end of buffer") {
      // The decoder must not mistake the trailing 128-byte "TAG" block for an
      // audio frame when scanning.
      const auto buf = make_mp3(128, 44100, 2, Tags::ID3V1, 5);

      const ma::MediaInfo info = decoder.decode(to_span(buf));

      CHECK(info.is_valid);
      CHECK(info.frame_count == 6); // 5 requested frames + 1 flush frame
      CHECK(info.sample_rate == 44100);
      CHECK(info.average_bitrate == 128);
    }

    SUBCASE("ID3v2 at start and ID3v1 at end") {
      // Both tag types present simultaneously — the common real-world layout.
      const auto buf = make_mp3(128, 44100, 2, Tags::ID3V1 | Tags::ID3V2, 5);

      const ma::MediaInfo info = decoder.decode(to_span(buf));

      CHECK(info.is_valid);
      CHECK(info.frame_count == 6); // 5 requested frames + 1 flush frame
      CHECK(info.sample_rate == 44100);
      CHECK(info.average_bitrate == 128);
    }

    SUBCASE("ID3v2 at start, Info CBR header, ID3v1 at end") {
      const auto buf = make_mp3(128, 44100, 2,
                                Tags::HEADERED | Tags::ID3V1 | Tags::ID3V2, 10);

      const ma::MediaInfo info = decoder.decode(to_span(buf));

      CHECK(info.is_valid);
      CHECK(info.frame_count == 11); // 10 requested frames + 1 flush frame
      CHECK(info.sample_rate == 44100);
      CHECK(info.average_bitrate == 128);
    }

    SUBCASE("ID3v2 at start, Xing VBR header, ID3v1 at end") {
      // Typical layout produced by LAME for VBR files:
      //   [ID3v2] [Xing frame] [audio frames …] [ID3v1]
      const auto buf =
          make_mp3(0, 44100, 2,
                   Tags::VBR | Tags::HEADERED | Tags::ID3V1 | Tags::ID3V2, 10);

      const ma::MediaInfo info = decoder.decode(to_span(buf));

      CHECK(info.is_valid);
      CHECK(info.frame_count == 11); // 10 requested frames + 1 flush frame
      CHECK(info.sample_rate == 44100);
      CHECK(info.average_bitrate > 0);
    }
  }
}

TEST_SUITE_END();
