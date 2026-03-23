#include "file_loader.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <string>

namespace ma = media_analyser;

static const std::filesystem::path samples_dir =
    std::filesystem::path{__FILE__}.parent_path() / "samples";

TEST_SUITE_BEGIN("integration");

TEST_CASE("IfstreamFileLoader::load") {
  ma::IfstreamFileLoader loader;

  SUBCASE("loads a text file and returns its bytes") {
    const auto result = loader.load(samples_dir / "hello.txt");

    const std::string content{reinterpret_cast<const char*>(result.data()),
                              result.size()};
    CHECK(content == "Hello, World!");
  }

  SUBCASE("loads an empty file and returns an empty vector") {
    const auto result = loader.load(samples_dir / "empty.txt");

    CHECK(result.empty());
  }

  SUBCASE("loads a binary file and returns correct bytes") {
    const auto result = loader.load(samples_dir / "binary.bin");

    REQUIRE(result.size() == 4);
    CHECK(result[0] == std::byte{0x01});
    CHECK(result[1] == std::byte{0x02});
    CHECK(result[2] == std::byte{0xFF});
    CHECK(result[3] == std::byte{0x00});
  }

  SUBCASE("throws FileLoaderError when file does not exist") {
    CHECK_THROWS_AS(loader.load(samples_dir / "nonexistent.txt"),
                    ma::FileLoaderError);
  }
}

TEST_SUITE_END;
