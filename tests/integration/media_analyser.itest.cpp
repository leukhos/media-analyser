#include "media_analyser.hpp"
#include "media_decoder.hpp"

#include <doctest/doctest.h>

#include <filesystem>
#include <memory>

namespace ma = media_analyser;

static const std::filesystem::path samples_dir =
    std::filesystem::path{__FILE__}.parent_path() / "samples";

// ISO 11172-4 conformance bitstreams (mp3/layer3/).
// Each file is the raw MPEG 1 Layer III audio from the test suite triplet
// (.mp3 input / .pcm reference / .f32 reference).
static const std::filesystem::path layer3_dir =
    samples_dir / "mp3" / "layer3";

// Real-world MP3 samples from the music-metadata project test suite.
// Used to cover metadata paths not present in the ISO suite.
static const std::filesystem::path mp3_dir = samples_dir / "mp3";

TEST_SUITE_BEGIN("integration");

TEST_CASE("MediaAnalyser::analyse") {
  auto analyser = ma::MediaAnalyser(std::make_shared<ma::Mp3MediaDecoder>());

  // ── ISO 11172-4 Layer III conformance files ───────────────────────────────

  // VBR across all 14 valid bitrates (32–320 kbps) at 32 kHz.
  // No Xing/VBRI tag — the decoder must walk all 150 frames.
  // Average bitrate = 21280 kbps-sum / 150 frames ≈ 141.9 kbps.
  SUBCASE("he_32khz — VBR all bitrates, 32 kHz") {
    const auto info = analyser.analyse(layer3_dir / "he_32khz.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 150);
    CHECK(info.sample_rate == 32000);
    CHECK(info.average_bitrate == doctest::Approx(141.9f).epsilon(0.001));
  }

  // Same VBR bitrate sweep at 44.1 kHz.
  // Larger frame sizes result in more frames (410) for the same content.
  SUBCASE("he_44khz — VBR all bitrates, 44.1 kHz") {
    const auto info = analyser.analyse(layer3_dir / "he_44khz.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 410);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(124.5f).epsilon(0.001));
  }

  // Same VBR bitrate sweep at 48 kHz.
  SUBCASE("he_48khz — VBR all bitrates, 48 kHz") {
    const auto info = analyser.analyse(layer3_dir / "he_48khz.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 150);
    CHECK(info.sample_rate == 48000);
    CHECK(info.average_bitrate == doctest::Approx(141.9f).epsilon(0.001));
  }

  // CBR 128 kbps, 44.1 kHz. Exercises all channel mode values
  // (stereo, joint stereo, dual channel, mono) across 128 frames.
  SUBCASE("he_mode — CBR 128 kbps, 44.1 kHz, channel-mode sweep") {
    const auto info = analyser.analyse(layer3_dir / "he_mode.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 128);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(128.0f).epsilon(0.001));
  }

  // CBR 128 kbps, 44.1 kHz, stereo. Covers the common encoder code paths
  // shared across the other he_* test cases.
  SUBCASE("hecommon — CBR 128 kbps, 44.1 kHz, stereo, common paths") {
    const auto info = analyser.analyse(layer3_dir / "hecommon.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 30);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(128.0f).epsilon(0.001));
  }

  // CBR 64 kbps, 48 kHz, mono. Broad Layer III compliance test —
  // exercises a wide range of encoder features in a single stream.
  SUBCASE("compl — CBR 64 kbps, 48 kHz, general compliance") {
    const auto info = analyser.analyse(layer3_dir / "compl.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 217);
    CHECK(info.sample_rate == 48000);
    CHECK(info.average_bitrate == doctest::Approx(64.0f).epsilon(0.001));
  }

  // CBR 64 kbps, 44.1 kHz, mono. Focuses on side information field
  // correctness (granule/channel bit allocations).
  SUBCASE("si — CBR 64 kbps, 44.1 kHz, side-information correctness") {
    const auto info = analyser.analyse(layer3_dir / "si.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 118);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(64.0f).epsilon(0.001));
  }

  // CBR 64 kbps, 44.1 kHz, mono. Exercises short and mixed block types
  // in the side information (window switching).
  SUBCASE("si_block — CBR 64 kbps, 44.1 kHz, short/mixed block types") {
    const auto info = analyser.analyse(layer3_dir / "si_block.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 64);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(64.0f).epsilon(0.001));
  }

  // CBR 64 kbps, 44.1 kHz, mono. Exercises boundary conditions in the
  // Huffman coding tables used for spectral data.
  SUBCASE("si_huff — CBR 64 kbps, 44.1 kHz, Huffman table edge cases") {
    const auto info = analyser.analyse(layer3_dir / "si_huff.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 75);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(64.0f).epsilon(0.001));
  }

  // Synthetic 1 kHz sine at 0 dB, CBR 128 kbps, joint stereo, no tags.
  // Complements the mono ISO files with a full joint-stereo stream.
  SUBCASE("sin1k0db — CBR 128 kbps, 44.1 kHz, joint stereo") {
    const auto info = analyser.analyse(layer3_dir / "sin1k0db.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 318);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(128.0f).epsilon(0.001));
  }

  // ── Real-world files with metadata ───────────────────────────────────────

  // CBR file with an Info (Xing-style) tag, no ID3 tags.
  // The decoder reads frame count and byte count from the tag directly,
  // bypassing the frame-walk (fast-path).
  SUBCASE("xing-info-cbr — Info tag (CBR Xing), no ID3") {
    const auto info = analyser.analyse(mp3_dir / "xing-info-cbr.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 4);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(128.0f).epsilon(0.001));
  }

  // CBR 128 kbps with an ID3v2.3 header, no Xing tag.
  // Exercises ID3v2 stripping followed by a full frame-walk.
  SUBCASE("id3v2.3-cbr-no-xing — ID3v2.3, no Xing tag") {
    const auto info = analyser.analyse(mp3_dir / "id3v2.3-cbr-no-xing.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 46);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(128.0f).epsilon(0.001));
  }

  // VBR file with an ID3v2.4 header and a Xing tag.
  // Exercises ID3v2.4 (the most recent ID3 version) stripping followed by
  // the Xing fast-path. avg_br is computed from Xing byte/frame counts.
  SUBCASE("id3v2.4-xing-vbr — ID3v2.4 + Xing VBR tag") {
    const auto info = analyser.analyse(mp3_dir / "id3v2.4-xing-vbr.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 41);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(33.8f).epsilon(0.001));
  }

  // CBR 256 kbps with an ID3v2.3 tag that contains a null-byte field
  // separator — exercises robustness of ID3v2 size parsing.
  SUBCASE("id3v2.3-null-separator — ID3v2.3 with null separator, no Xing") {
    const auto info = analyser.analyse(mp3_dir / "id3v2.3-null-separator.mp3");
    CHECK(info.is_valid);
    CHECK(info.frame_count == 10);
    CHECK(info.sample_rate == 44100);
    CHECK(info.average_bitrate == doctest::Approx(256.0f).epsilon(0.001));
  }

  // ── Unsupported formats — must return is_valid false ─────────────────────

  SUBCASE("fl2 — MPEG 1 Layer I must be rejected") {
    const auto info = analyser.analyse(mp3_dir / "layer1" / "fl2.mp1");
    CHECK_FALSE(info.is_valid);
  }

  SUBCASE("fl11 — MPEG 1 Layer II must be rejected") {
    const auto info = analyser.analyse(mp3_dir / "layer2" / "fl11.mp2");
    CHECK_FALSE(info.is_valid);
  }

  SUBCASE("mpeg2-layer3 — MPEG 2 Layer III must be rejected") {
    const auto info = analyser.analyse(mp3_dir / "mpeg2-layer3.mp3");
    CHECK_FALSE(info.is_valid);
  }
}

TEST_SUITE_END();
