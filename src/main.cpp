#include "media_analyser.hpp"
#include "media_decoder.hpp"

#include <iostream>

namespace ma = media_analyser;

int main(int argc, char** argv) {

  auto mp3_analyser = ma::MediaAnalyser(
      std::make_shared<ma::Mp3MediaDecoder>());

  if (mp3_analyser.is_supported(argv[1])) {
    auto details = mp3_analyser.analyse(argv[1]);

    std::cout << details.frame_count << std::endl;
  }

  return 0;
}
