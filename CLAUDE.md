# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A command-line C++ tool (MPEG 1 Layer III only) that reads an MP3 file and reports frame count, sample rate, average bitrate, and CBR/VBR status.

## Build Commands

```bash
# Development (debug + tests) — most common workflow
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

# Run a single test by name pattern
ctest --preset dev -R "MyTestName"

# Optimised test build (used in CI)
cmake --workflow --preset test

# Release build
cmake --preset release && cmake --build --preset release

# Benchmarks
cmake --preset bench && cmake --build --preset bench

# Format all source files
clang-format -i src/**/*.{cpp,h} tests/**/*.{cpp,h} benches/**/*.cpp
```

Build outputs go to `build/<preset>/`. The `dev` preset is already configured in `build/dev/`.

## Architecture

All code lives in the `media_analyser` namespace. The public interfaces (`include/`) use the `I` prefix for service contracts designed for DI/mocking.

```
main.cpp
  └── MediaAnalyser        (orchestrates; never throws — catches internally)
        ├── IFileLoader    (abstraction over file I/O)
        │     └── IfstreamFileLoader  (concrete: reads via std::ifstream)
        ├── IMediaDecoder  (abstraction over format decoding)
        │     └── Mp3MediaDecoder     (concrete: MPEG 1 Layer III only)
        └── ILogger        (abstraction over logging)
              └── StderrLogger        (concrete: writes to stderr)
```

**Key types** (`include/media_types.hpp`):
- `ByteSpan` — non-owning `{const std::byte*, size_t}` view passed to the decoder
- `MediaInfo` — result struct: `is_valid`, `frame_count`, `sample_rate`, `average_bitrate`

**Error handling contract**:
- `IMediaDecoder::is_supported(ByteSpan)` — pure predicate, never throws
- `IMediaDecoder::decode(ByteSpan)` — throws `DecodeError` (a `std::runtime_error`) on bad input
- `MediaAnalyser` catches all exceptions internally; its public methods never throw
- `main.cpp` needs no try/catch — it calls `is_supported`, checks the return, then calls `analyse`

**Test injection**: When built with `MEDIA_ANALYSER_TEST` defined, `MediaAnalyser` exposes `set_logger()` and `set_file_loader()` setters for injecting mocks in tests (guarded by `#ifdef MEDIA_ANALYSER_TEST`).

## Testing

Tests use **DocTest** + **Trompeloeil** (mocking). The test entry point is `tests/main.cpp`.

| Test type | Location | File suffix | What it tests |
|-----------|----------|-------------|---------------|
| Unit | `tests/unit/` | `.utest.cpp` | Public API with mocks/byte arrays — no real files |
| Integration | `tests/integration/` | `.itest.cpp` | Real files on disk |

Test case naming: `TEST_CASE("Module - Scenario")`, subcases use `SUBCASE("specific scenario")`.
All tests follow the Arrange-Act-Assert pattern.

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `MediaAnalyser` |
| Functions/Methods | snake_case | `is_supported()` |
| Member variables | `m_` prefix | `m_decoder` |
| Global variables | `g_` prefix | `g_debug_mode` |
| Constants | SCREAMING_SNAKE_CASE | `MAX_BUFFER_SIZE` |
| Service interfaces | `I` prefix | `ILogger`, `IMediaDecoder` |
| Capability interfaces | `-able` suffix | `Drawable` |
| Error types | `Error` suffix | `DecodeError` |
| Files | snake_case | `media_decoder.cpp` |
| CMake targets | kebab-case | `media-analyser-tests` |
| CMake variables | SCREAMING_SNAKE_CASE | `MEDIA_ANALYSER_ENABLE_TEST` |

## Code Style

- LLVM clang-format style, 2-space indent, 80-character line limit (`.clang-format` at root)
- Header inclusion order (each group separated by a blank line, sorted alphabetically within):
  1. First-party headers
  2. Third-party headers
  3. Standard library headers
- No implicit or transitive includes — every dependency must be explicitly included
- Use forward declarations in headers where possible

## Dependencies (via vcpkg)

- **DocTest** — header-only test framework (`test` feature)
- **Trompeloeil** — C++ mocking framework (`test` feature)
- **Google Benchmark** — benchmarking (`benchmark` feature)

vcpkg dependencies are only installed when the corresponding CMake feature is enabled via `VCPKG_MANIFEST_FEATURES`.
