# media-analyser

A command-line C++ tool that reads an MP3 file and reports key audio metadata:

- Frame count
- Sample rate (Hz)
- Average bitrate (kbps)

Supports MPEG 1 Layer III only. Handles both CBR and VBR files, including raw VBR streams without a Xing/VBRI header.

## Usage

```bash
media-analyser <file.mp3>
```

## Project Structure

```
media-analyser/
├── CMakeLists.txt              # Main CMake configuration
├── CMakePresets.json           # CMake presets (dev, test, release, bench)
├── vcpkg.json                  # vcpkg dependency manifest
├── include/                    # Public headers
│   ├── media_analyser.hpp      # MediaAnalyser — orchestrates loading and decoding
│   ├── media_decoder.hpp       # IMediaDecoder interface + Mp3MediaDecoder
│   ├── file_loader.hpp         # IFileLoader interface + IfstreamFileLoader
│   ├── logger.hpp              # ILogger interface + StdErrLogger
│   └── media_types.hpp         # ByteSpan, MediaInfo result type
├── src/                        # Implementation
│   ├── main.cpp                # CLI entry point
│   ├── media_analyser.cpp
│   ├── media_decoder.cpp
│   ├── file_loader.cpp
│   └── logger.cpp
├── tests/                      # DocTest test suite
│   ├── main.cpp                # DocTest entry point
│   ├── unit/                   # Unit tests (mocks and byte arrays — no real files)
│   └── integration/            # Integration tests (real files on disk)
├── benches/                    # Google Benchmark (not yet implemented)
└── docs/                       # Documentation
```

## Building

The project uses purpose-based CMake presets with automatic platform detection. All presets use the Ninja generator and optional vcpkg integration (when `VCPKG_ROOT` is set).

| Preset    | Build type      | Tests | Description                         |
|-----------|-----------------|-------|-------------------------------------|
| `dev`     | Debug           | ON    | Development with full debug symbols |
| `test`    | RelWithDebInfo  | ON    | Test execution with optimisations   |
| `release` | Release         | OFF   | Production build                    |

```bash
# Development (debug + tests)
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

# Run a single test by name pattern
ctest --preset dev -R "MyTestName"

# Optimised test build (used in CI)
cmake --workflow --preset test

# Production release
cmake --preset release
cmake --build --preset release
```

## CMake Options

| Option                       | Default | Description          |
|------------------------------|---------|----------------------|
| `MEDIA_ANALYSER_ENABLE_TEST` | OFF     | Build the test suite |

## Dependencies

Managed via vcpkg manifest features — only installed when the corresponding feature is enabled:

- **[doctest](https://github.com/doctest/doctest)** — lightweight, header-only C++ testing framework (`test` feature)
- **[Trompeloeil](https://github.com/rollbear/trompeloeil)** — header-only C++14 mocking framework (`test` feature)
- **[LAME](https://lame.sourceforge.io)** (`mp3lame`) — MP3 encoder used in integration tests to generate test fixtures (`test` feature)

## Platform Support

- **Linux**: GCC with `-Wall -Wextra -Wpedantic`
- **macOS**: Clang with `-Wall -Wextra -Wpedantic`
- **Windows**: MSVC with `/W4 /permissive- /EHsc`

## Code Guidelines

- **Naming conventions**: [docs/naming_conventions.md](docs/naming_conventions.md)
- **Code formatting**: [docs/code_guidelines.md](docs/code_guidelines.md) — LLVM style, 2-space indent, 80-column limit

## Potential Improvements

### File mapping instead of file loading

Replace `IfstreamFileLoader` (which reads the entire file into a heap-allocated buffer) with a memory-mapped implementation: `mmap`/`munmap` on Linux/macOS and `CreateFileMapping`/`MapViewOfFile` on Windows. This avoids the copy into userspace and lets the OS page in only the regions the decoder actually touches, which matters for large files.

### C++23 migration

The codebase currently targets C++17. Candidates for modernisation:

- **`std::span`** — replace `ByteSpan` (`{const std::byte*, size_t}`) with `std::span<const std::byte>`, which carries the same non-owning semantics with standard vocabulary type support.
- **`std::expected`** — replace the throw/catch contract on `IMediaDecoder::decode` with `std::expected<MediaInfo, MediaDecoderError>`, making the error path explicit in the return type and removing the need for `try`/`catch` in `MediaAnalyser`.
- **`std::print` / `std::println`** — replace `fprintf`/`std::cerr` calls in the logger and CLI with the type-safe `<print>` facilities.

## License

MIT License — see [LICENSE](LICENSE) for details.
