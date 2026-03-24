#include "media_analyser.hpp"
#include "file_loader.hpp"
#include "logger.hpp"
#include "media_decoder.hpp"
#include "media_types.hpp"

#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

namespace ma = media_analyser;

class MockLogger : public trompeloeil::mock_interface<ma::ILogger> {
  IMPLEMENT_MOCK2(log);
};

class MockFileLoader : public trompeloeil::mock_interface<ma::IFileLoader> {
  IMPLEMENT_MOCK1(load);
};

class MockMediaDecoder : public trompeloeil::mock_interface<ma::IMediaDecoder> {
  IMPLEMENT_MOCK1(decode);
};

TEST_SUITE_BEGIN("unit");

TEST_CASE("MediaAnalyser::analyse") {
  auto mock_logger = std::make_shared<MockLogger>();
  auto mock_file_loader = std::make_shared<MockFileLoader>();
  auto mock_decoder = std::make_shared<MockMediaDecoder>();
  auto analyser = ma::MediaAnalyser(mock_decoder);
  analyser.set_logger(mock_logger);
  analyser.set_file_loader(mock_file_loader);

  const std::filesystem::path fake_path{"path"};
  const std::vector<std::byte> fake_bytes{std::byte{0x01}, std::byte{0x02}};

  SUBCASE("FileLoader throws — logs Error with 'FileLoader', returns invalid "
          "MediaInfo") {
    ALLOW_CALL(*mock_file_loader, load(fake_path))
        .THROW(ma::FileLoaderError("load failed"));

    REQUIRE_CALL(*mock_logger,
                 log(ma::LogLevel::Error, trompeloeil::re("FileLoader")));

    const ma::MediaInfo result = analyser.analyse(fake_path);

    CHECK_FALSE(result.is_valid);
  }

  SUBCASE("decoder throws MediaDecoderError — logs Error with 'MediaDecoder', "
          "returns invalid MediaInfo") {
    ALLOW_CALL(*mock_file_loader, load(fake_path)).RETURN(fake_bytes);
    ALLOW_CALL(*mock_decoder, decode(trompeloeil::_))
        .THROW(ma::MediaDecoderError("bad frame"));

    REQUIRE_CALL(*mock_logger,
                 log(ma::LogLevel::Error, trompeloeil::re("MediaDecoder")));

    const ma::MediaInfo result = analyser.analyse(fake_path);

    CHECK_FALSE(result.is_valid);
  }

  SUBCASE("decoder throws unexpected exception — logs Error, returns invalid "
          "MediaInfo") {
    ALLOW_CALL(*mock_file_loader, load(fake_path)).RETURN(fake_bytes);
    ALLOW_CALL(*mock_decoder, decode(trompeloeil::_))
        .THROW(std::runtime_error("unexpected"));

    REQUIRE_CALL(*mock_logger, log(ma::LogLevel::Error, trompeloeil::_));

    const ma::MediaInfo result = analyser.analyse(fake_path);

    CHECK_FALSE(result.is_valid);
  }

  SUBCASE("success — returns MediaInfo from decoder, no logging") {
    const ma::MediaInfo expected{true, 100, 44100, 128.0f};
    ALLOW_CALL(*mock_logger, log(trompeloeil::_, trompeloeil::_));
    ALLOW_CALL(*mock_file_loader, load(fake_path)).RETURN(fake_bytes);
    ALLOW_CALL(*mock_decoder, decode(trompeloeil::_)).RETURN(expected);

    const ma::MediaInfo result = analyser.analyse(fake_path);

    CHECK(result.is_valid);
    CHECK(result.frame_count == 100);
    CHECK(result.sample_rate == 44100);
    CHECK(result.average_bitrate == doctest::Approx(128.0f).epsilon(0.001));
  }
}

TEST_SUITE_END();
