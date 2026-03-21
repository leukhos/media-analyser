#pragma once

#include "media_types.hpp"

#include <stdexcept>
#include <string>

namespace media_analyser {

class DecodeError : public std::runtime_error {
public:
  explicit DecodeError(const std::string& message)
      : std::runtime_error{message} {}
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
  void check(ByteSpan buffer) const; // Throws DecodeError with diagnostics
};

} // namespace media_analyser
