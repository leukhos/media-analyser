# MP3 Analyser — Design Document
*C++ Implementation Notes & Decisions*

---

## 1. Project Overview

A simple command-line C++ tool to extract metadata from MP3 files. The extractor reports three pieces of information:

- Number of frames
- Sample rate (Hz)
- Average bitrate (kbps) and whether the file is CBR or VBR

**Scope constraint:** MPEG 1 Layer III only. MPEG 2 and MPEG 2.5 files are explicitly rejected.

---

## 2. MP3 Frame Structure

Every MP3 frame begins with a 4-byte header. For MPEG 1 Layer III files, the first two bytes always follow the same pattern:

```
Byte 0: 0xFF
Byte 1: 0xFB  (no CRC)  or  0xFA  (CRC protected)
        = 1111 101x  in binary
          ^^^^ ^^    MPEG1 (11), Layer III (01)
```

Bytes 2 and 3 carry the variable fields:

| Bits | Field            | Values        | Notes                                    |
|------|------------------|---------------|------------------------------------------|
| 4    | Bitrate index    | 1–14 valid    | Index 0 = free-format, 15 = forbidden    |
| 2    | Sample rate index| 0, 1, 2 valid | Index 3 = reserved                       |
| 1    | Padding          | 0 or 1        | Adds 1 byte to frame size when set       |
| 1    | Private          | ignored       |                                          |
| 2    | Channel mode     | 0–3           | 3 = Mono, others = stereo variants       |

### 2.1 Lookup Tables

Only MPEG 1 Layer III values are required:

```cpp
// Bitrate table (kbps) — index 0 and 15 are invalid
int BITRATE[]    = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};

// Sample rate table (Hz)
int SAMPLERATE[] = {44100, 48000, 32000, 0};  // index 3 = reserved
```

### 2.2 Frame Size Formula

The frame size in bytes (used to advance to the next frame) is:

```
frame_size = 144 * bitrate_kbps * 1000 / sample_rate + padding
```

The constant `144` is specific to MPEG 1 Layer III (1152 samples/frame ÷ 8 ÷ 1000).
Layer I and Layer II use different constants and can be ignored.

### 2.3 CBR vs VBR Detection

| Method        | How to detect                                                                                                                          |
|---------------|----------------------------------------------------------------------------------------------------------------------------------------|
| Xing/Info tag | Present in the audio data of the first frame, after the side information. `Xing` = VBR, `Info` = CBR with Xing-style header.          |
| VBRI tag      | Fraunhofer VBR header. Located at a fixed offset of 32 bytes into the first frame's audio data.                                        |
| No tag        | Walk all frames. If any two frames have different sizes, the file is VBR. `he_32khz.bit` falls into this category.                     |

Side information size (needed to locate Xing/VBRI offset):

```cpp
side_info_size = (channel_mode == 3) ? 17 : 32;  // mono : stereo/joint/dual
```

---

## 3. Conformance Test File — he_32khz.bit

This file is from the ISO 11172-4 MP3 conformance test suite. Each test case ships as a triplet:

| File  | Purpose                                                    |
|-------|------------------------------------------------------------|
| `.bit` | The MP3 bitstream — input for the analyser                |
| `.pcm` | Reference decoded audio, 16-bit signed integer PCM, mono  |
| `.f32` | Reference decoded audio, 32-bit float PCM, mono           |

The `.pcm` and `.f32` files are used to validate MP3 *decoders*, not analysers. Only the `.bit` file is relevant here.

### 3.1 Expected Analyser Output

| Field           | Expected value                                           |
|-----------------|----------------------------------------------------------|
| MPEG version    | MPEG 1                                                   |
| Layer           | Layer III                                                |
| Sample rate     | 32 000 Hz                                                |
| Channel mode    | Mono                                                     |
| Frame count     | 150                                                      |
| CBR / VBR       | VBR — no Xing/Info/VBRI tag present                      |
| Average bitrate | ~142 kbps (computed by scanning all frames)              |
| Frame size range| 144 to 1 440 bytes (14 distinct sizes)                   |

> Because no Xing/VBRI tag is present, the analyser must walk all 150 frames to obtain
> the frame count and average bitrate. There is no shortcut.

---

## 4. Architecture

### 4.1 Class Split

The implementation is divided into two classes with distinct responsibilities:

| Class        | Responsibility                                               | Testability                         |
|--------------|--------------------------------------------------------------|-------------------------------------|
| `Mp3Parser`  | Pure parsing logic. Works on raw byte buffers. No file I/O. | Unit tests — no files needed        |
| `Mp3Analyser`| File I/O only. Reads a file, delegates to `Mp3Parser`.      | Integration tests with real files   |

This separation follows the Single Responsibility Principle and makes `Mp3Parser` reusable
for non-file sources (network streams, memory buffers) without any modification.

### 4.2 Public Interfaces

```cpp
// Pure parsing — fully unit testable
class Mp3Parser {
public:
    bool       is_valid(const std::uint8_t* data, std::size_t size);
    Mp3Details get_details(const std::uint8_t* data, std::size_t size);
};

// File I/O facade — integration tested
class Mp3Analyser {
public:
    explicit Mp3Analyser(std::shared_ptr<ILogger> logger);

    bool       is_valid(const std::string& filename) noexcept;
    Mp3Details get_details(const std::string& filename) noexcept;

private:
    Mp3Parser                mp3_parser_;
    std::shared_ptr<ILogger> logger_;
};
```

### 4.3 Mp3Details

The result type returned by `get_details`. Contains a validity flag so callers who skip
`is_valid` can still detect failure:

```cpp
struct Mp3Details {
    bool is_valid    = false;
    int  frame_count = 0;
    int  sample_rate = 0;   // Hz
    int  avg_bitrate = 0;   // kbps
    bool is_vbr      = false;
};
```

If `get_details` is called on an invalid or unreadable file, it returns a
default-constructed `Mp3Details` (`is_valid = false`) rather than throwing.

---

## 5. Error Handling

### 5.1 Strategy

| Layer          | Behaviour                                                                                      |
|----------------|------------------------------------------------------------------------------------------------|
| `Mp3Parser`    | Throws `std::exception` (or subclasses) on malformed data, invalid headers, unsupported format.|
| `Mp3Analyser`  | Catches all exceptions. Never throws. Logs the error and returns a safe fallback value.         |
| Client (`main`)| No try/catch required. Calls `is_valid`, checks the return value, then calls `get_details`.    |

The `noexcept` specifier is applied to both `Mp3Analyser` public methods, making the
no-throw guarantee explicit and compiler-enforced.

### 5.2 Logger Interface & Default Implementation

```cpp
enum class LogLevel { Debug, Info, Warning, Error };

/**
 * @brief Interface for logging mechanisms.
 *
 * Currently implemented by StderrLogger as the default behaviour.
 *
 * To extend the logging mechanism, the Decorator design pattern is a relevant
 * technique: additional loggers (e.g. file logger, timestamp logger, level
 * filter) can be implemented as decorators wrapping any ILogger, and composed
 * at construction time without modifying existing classes.
 *
 * Example of how a decorator chain would look:
 * @code
 *   auto logger = std::make_shared<TimestampLogger>(
 *                     std::make_shared<StderrLogger>()
 *                 );
 * @endcode
 */
class ILogger {
public:
    virtual void log(LogLevel level, const std::string& message) = 0;
    virtual ~ILogger() = default;
};

/** @brief Default implementation — writes all messages to stderr. */
class StderrLogger : public ILogger {
public:
    void log(LogLevel level, const std::string& message) override {
        std::cerr << "[" << to_string(level) << "] " << message << "\n";
    }

private:
    static std::string to_string(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error:   return "ERROR";
        }
    }
};
```

### 5.3 Client Usage Pattern

```cpp
int main(int argc, char* argv[]) {
    auto logger = std::make_shared<StderrLogger>();
    Mp3Analyser analyser(logger);

    std::string filename = argv[1];

    if (!analyser.is_valid(filename))
        return 1;  // error already logged inside Mp3Analyser

    auto details = analyser.get_details(filename);
    // use details ...
}
```

---

## 6. Testing Strategy

### 6.1 Unit Tests — Mp3Parser

No files required. Test cases are constructed as raw byte arrays in test code:

```cpp
// Valid MPEG1 Layer III header: 128 kbps, 44100 Hz, no padding, mono
const uint8_t valid[]    = { 0xFF, 0xFB, 0x90, 0xC0 };

// Bad sync word
const uint8_t bad_sync[] = { 0xFE, 0xFB, 0x90, 0xC0 };

// MPEG2 — must be rejected
const uint8_t mpeg2[]    = { 0xFF, 0xF3, 0x90, 0xC0 };

// Invalid bitrate index 15
const uint8_t bad_br[]   = { 0xFF, 0xFB, 0xF0, 0xC0 };

// Free-format bitrate index 0
const uint8_t free_fmt[] = { 0xFF, 0xFB, 0x00, 0xC0 };
```

Coverage targets:

- Valid header accepted
- Wrong sync word rejected
- MPEG 2 / MPEG 2.5 rejected
- Invalid bitrate indices 0 and 15 rejected
- Invalid sample rate index 3 rejected
- Correct sample rate extracted for each index (44100, 48000, 32000)
- Correct bitrate extracted for each valid index
- Frame size formula correct with and without padding
- CBR detected when all frame sizes are equal
- VBR detected when frame sizes differ

### 6.2 Integration Tests — Mp3Analyser

Use real files. Coverage targets:

- ID3v2 tag at start is skipped correctly
- ID3v1 tag at end does not produce a phantom frame
- Raw VBR file without Xing tag (`he_32khz.bit`) produces correct frame count
- CBR file with Info tag correctly identified as CBR
- Empty or truncated file handled gracefully
- Non-MP3 file correctly rejected by `is_valid`

### 6.3 Test File Sources

| Source                                         | Purpose                                                            |
|------------------------------------------------|--------------------------------------------------------------------|
| ISO 11172-4 conformance suite (`he_32khz.bit`) | Raw VBR without Xing tag — the harder parsing case                 |
| LAME-generated samples                         | Known-good files with controlled parameters                        |
| FFmpeg fate suite (`rsync://fate-suite.ffmpeg.org`) | Edge cases: corrupt headers, unusual encodings              |
| eyeD3 test data (GitHub)                       | Real-world files with missing Xing tags and unusual encodings      |

Generate LAME samples with known expected values:

```bash
# CBR 128 kbps, 44100 Hz, stereo
lame --cbr -b 128 input.wav cbr_128_44100.mp3

# VBR with Xing header
lame -V 2 input.wav vbr_xing.mp3

# Mono, low bitrate
lame --cbr -b 64 -m m input.wav cbr_64_mono.mp3

# Verify expected values
ffprobe -v quiet -print_format json -show_streams cbr_128_44100.mp3
```

---

## 7. CLI Library

Three candidates were evaluated from the awesome-cpp list. All are header-only and cross-platform:

| Library   | C++ std | Licence | Help output | Recommendation                                      |
|-----------|---------|---------|-------------|-----------------------------------------------------|
| CLI11     | C++11   | BSD     | Excellent   | Best long-term choice — widely adopted, rich feature set |
| cxxopts   | C++11   | MIT     | Clean       | Simplest option — ideal for 1–2 parameters          |
| argparse  | C++17   | MIT     | Polished    | Python-like API — good if C++17 is available        |

For this project (two parameters, simple help message), **cxxopts** is the path of least
resistance. **CLI11** is the safer choice if the project is expected to grow.

---

## 8. Quick Reference

### 8.1 MPEG 1 Layer III — Header Byte 1

| Value              | Meaning                              |
|--------------------|--------------------------------------|
| `0xFB` (1111 1011) | MPEG1, Layer III, no CRC — most common pattern |
| `0xFA` (1111 1010) | MPEG1, Layer III, CRC protected      |
| `0xF3`, `0xF2`     | MPEG2 — reject                       |
| `0xE3`, `0xE2`     | MPEG2.5 — reject                     |

### 8.2 he_32khz.bit at a Glance

| Property                  | Value                              |
|---------------------------|------------------------------------|
| Format                    | MPEG 1 Layer III, Mono             |
| Sample rate               | 32 000 Hz                          |
| Frame count               | 150                                |
| CBR / VBR                 | VBR — no Xing/VBRI tag             |
| Frame sizes               | 144 to 1 440 bytes (14 distinct)   |
| Average bitrate           | ~142 kbps                          |
| First frame header (hex)  | `FF FB 18 C0`                      |
