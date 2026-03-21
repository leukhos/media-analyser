#include "media_analyser.hpp"
#include "media_decoder.hpp"

#include <filesystem>
#include <iostream>

namespace ma = media_analyser;

int main(int argc, char** argv) {

  auto mp3_analyser =
      ma::MediaAnalyser(std::make_shared<ma::Mp3MediaDecoder>());

  const std::filesystem::path input{argv[1]};
  auto details = mp3_analyser.analyse(input);
  if (details.is_valid) {
    std::cout << details.frame_count << std::endl;
  }

  return 0;
}
