#include "media_analyser.hpp"
#include "media_decoder.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>

namespace ma = media_analyser;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: media-analyser <file>\n";
    return 1;
  }

  auto mp3_analyser =
      ma::MediaAnalyser(std::make_shared<ma::Mp3MediaDecoder>());

  const std::filesystem::path input{argv[1]};
  auto media_info = mp3_analyser.analyse(input);
  if (media_info.is_valid) {
    std::cout << "MP3 details:\n"
              << "\tFrames       : " << media_info.frame_count << '\n'
              << "\tSample rate  : " << media_info.sample_rate << '\n'
              << "\tAvg bit rate : " << std::fixed << std::setprecision(1)
              << media_info.average_bitrate << std::endl;
  }

  return 0;
}
