# media-analyser

A command-line C++ tool that reads an MP3 file and reports key audio metadata:

- Frame count
- Sample rate (Hz)
- Average bitrate (kbps)
- Whether the file is CBR or VBR

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
│   ├── file_loader.hpp         # IFileLoader interface (memory-mapped file I/O)
│   ├── logger.hpp              # ILogger interface + StderrLogger
│   └── media_types.hpp         # ByteSpan, MediaInfo result type
├── src/                        # Implementation
│   ├── main.cpp                # CLI entry point
│   ├── media_analyser.cpp
│   ├── media_decoder.cpp
│   ├── file_loader.cpp
│   └── logger.cpp
├── tests/                      # DocTest test suite
│   ├── main.cpp                # DocTest entry point
│   └── unit/                   # Unit tests (no files required)
├── benches/                    # Google Benchmark
└── docs/                       # Documentation
```

## Building

The project uses purpose-based CMake presets with automatic platform detection. All presets use the Ninja generator and optional vcpkg integration (when `VCPKG_ROOT` is set).

| Preset    | Build type      | Tests | Benchmarks | Description                        |
|-----------|-----------------|-------|------------|------------------------------------|
| `dev`     | Debug           | ON    | OFF        | Development with full debug symbols |
| `test`    | RelWithDebInfo  | ON    | OFF        | Test execution with optimisations   |
| `release` | Release         | OFF   | OFF        | Production build                    |
| `bench`   | Release         | OFF   | ON         | Performance benchmarking            |

```bash
# Development (debug + tests)
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

# Production release
cmake --preset release
cmake --build --preset release

# Benchmarking
cmake --preset bench
cmake --build --preset bench
```

## CMake Options

| Option                        | Default | Description              |
|-------------------------------|---------|--------------------------|
| `MEDIA_ANALYSER_ENABLE_TEST`  | OFF     | Build the test suite     |
| `MEDIA_ANALYSER_ENABLE_BENCH` | OFF     | Build benchmarks         |

## Dependencies

Managed via vcpkg manifest features — only installed when the corresponding feature is enabled:

- **DocTest** — lightweight header-only testing framework (`test` feature)
- **Trompeloeil** — C++ mocking framework (`test` feature)
- **Google Benchmark** — performance benchmarking (`bench` feature)

## Platform Support

- **Linux**: GCC with `-Wall -Wextra -Wpedantic`
- **macOS**: Clang with `-Wall -Wextra -Wpedantic`
- **Windows**: MSVC with `/W4 /permissive- /EHsc`

## Code Guidelines

- **Naming conventions**: [docs/naming_conventions.md](docs/naming_conventions.md)
- **Code formatting**: [docs/code_guidelines.md](docs/code_guidelines.md) — LLVM style, 2-space indent, 80-column limit

## License

MIT License — see [LICENSE](LICENSE) for details.
