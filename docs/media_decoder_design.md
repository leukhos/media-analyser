# Media Decoder — C++17 Design Summary

## Overview

A cross-platform MP3 media decoder structured around clear separation of concerns, with idiomatic C++17 error handling and memory-mapped file I/O.

---

## Class Structure

```
MediaAnalyser
├── uses → Mp3Decoder (injected by reference)
└── uses → MappedFile (created per call, RAII)

Mp3Decoder
├── is_supported(BufferView) noexcept → bool
├── decode(BufferView) → MediaInfo        [throws DecodeError]
└── check(BufferView)  [private, throws DecodeError with diagnostics]

MappedFile
└── view() noexcept → BufferView

BufferView
├── const std::byte* data
└── size_t size
```

---

## Key Design Decisions

### Error Handling

Two distinct patterns are used, each with a clear role:

- **`is_supported`** — pure predicate, `noexcept`, no diagnostics, returns `bool`. Used for branching and CLI parameter validation.
- **`decode`** — throws `DecodeError` (a `std::runtime_error` subclass) with rich diagnostic messages. Internally delegates to a private `check()` function.

A standalone `void check()` as a public API was deliberately avoided — it would only ever return or throw, making the return type meaningless. It lives as a private helper instead.

### Buffer Representation

The decoder interface uses a lightweight non-owning view:

```cpp
struct BufferView {
    const std::byte* data;
    size_t           size;
};
```

- `std::byte` is preferred over `char` or `uint8_t` — semantically correct for raw memory
- Non-owning view keeps the decoder completely decoupled from how data was loaded
- In C++20 this becomes `std::span<const std::byte>` — a one-line substitution

#### Why `void*` Internally in `MappedFile`

Both OS APIs (`mmap` on POSIX, `MapViewOfFile` on Windows) return `void*` — the OS has no concept of what type of data lives in the mapped region. Storing the pointer as `void*` internally avoids a cast at the point of assignment, and correctly signals that this is an opaque, OS-managed memory region rather than an owned byte array.

The cast happens exactly once, at the public boundary:

```cpp
[[nodiscard]] std::span<const uint8_t> bytes() const noexcept
{
    return { static_cast<const uint8_t*>(data_), size_ };
//            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//            Single, explicit, controlled conversion point
}
```

Three-layer mental model:

```
┌─────────────────────────────────────────────────────┐
│  OS layer        void*   — untyped mapped region     │
│                  ↓ stored as-is in data_             │
├─────────────────────────────────────────────────────┤
│  C++ boundary    static_cast<const uint8_t*>         │
│                  ↓ cast once at public interface     │
├─────────────────────────────────────────────────────┤
│  Consumer        std::span<const uint8_t>            │
│                  — typed, bounds-safe, no raw ptr    │
└─────────────────────────────────────────────────────┘
```

`std::byte*` was considered as an internal type but rejected: it still requires a `reinterpret_cast` from `void*` on assignment, and minimp3 expects `uint8_t*` at its interface anyway — so it adds friction at both ends without meaningful gain.

### File Loading — Memory-Mapped I/O

Rather than reading the entire file into a `std::vector`, the file is memory-mapped:

| Approach | Verdict |
|---|---|
| `std::ifstream::read` | Acceptable but requires `reinterpret_cast`; text-oriented API |
| `std::vector<std::byte>` | Simple ownership, but copies entire file into RAM |
| `std::FILE` + `fread` | Honest about binary I/O, takes `void*`, but legacy C |
| `open()` / `read()` (POSIX `fcntl.h`) | Lower level than `fopen`; talks directly to kernel; POSIX-only |
| `mmap` (POSIX) / `MapViewOfFile` (Windows) | Best for media files — no copy, lazy paging, OS-managed |

`MappedFile` wraps the platform-specific API in a RAII class behind a unified interface. The decoder never knows which approach was used — it only sees a `BufferView`.

#### Note on `fcntl.h`

`fcntl.h` is a POSIX header (Linux, macOS, FreeBSD) that provides `::open`, `O_RDONLY`, and `::read`. It does not exist on Windows. It is used internally by the POSIX branch of `MappedFile` to obtain a file descriptor before calling `mmap`. The `read()` system call requires a loop to guard against short reads — it is not guaranteed to consume the full buffer in a single call.

### Dependency Injection

`Mp3Decoder` is injected into `MediaAnalyser` by reference rather than owned as a concrete member:

```cpp
class MediaAnalyser {
public:
    explicit MediaAnalyser(Mp3Decoder& decoder);
    ...
private:
    Mp3Decoder& m_decoder;
};
```

This makes `MediaAnalyser` easier to test (mock decoder) and easier to extend (swap decoder without changing the analyser).

`FileLoader` as a dedicated class was discarded — it had no state, so a free function `load_file()` is more honest. In practice this responsibility was absorbed into `MappedFile`.

---

## File Layout

| File | Responsibility |
|---|---|
| `BufferView.h` | Non-owning view over raw bytes |
| `MappedFile.h` | Cross-platform RAII memory-mapped file |
| `Mp3Decoder.h/.cpp` | MP3 format validation and decoding |
| `MediaAnalyser.h/.cpp` | Orchestrates loading and decoding |
| `main.cpp` | CLI entry point with parameter validation |

---

## Cross-Platform `MappedFile`

The class exposes a single unified interface on all platforms. Platform-specific code is entirely contained within `#ifdef _WIN32` blocks.

```cpp
class MappedFile
{
public:
    explicit MappedFile(const std::filesystem::path& path);
    ~MappedFile();

    MappedFile(const MappedFile&)            = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    MappedFile(MappedFile&&) noexcept;
    MappedFile& operator=(MappedFile&&) noexcept;

    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept;
    [[nodiscard]] size_t size()  const noexcept;
    [[nodiscard]] bool   empty() const noexcept;

private:
    const void* data_ = nullptr;   // void* matches OS API return type exactly
    size_t      size_ = 0;

#ifdef _WIN32
    HANDLE file_handle_    = INVALID_HANDLE_VALUE;
    HANDLE mapping_handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};
```

### POSIX Implementation (`mmap`)

```cpp
// Requires: <fcntl.h>, <sys/mman.h>, <sys/stat.h>, <unistd.h>

fd_ = ::open(path.c_str(), O_RDONLY);
struct stat st{};
::fstat(fd_, &st);
size_ = static_cast<size_t>(st.st_size);

data_ = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
::madvise(const_cast<void*>(data_), size_, MADV_SEQUENTIAL); // hint to kernel

// Teardown
::munmap(const_cast<void*>(data_), size_);
::close(fd_);
```

### Windows Implementation (`MapViewOfFile`)

```cpp
// Requires: <windows.h>

file_handle_ = ::CreateFileW(
    path.wstring().c_str(),   // wide string for correct Unicode support
    GENERIC_READ,
    FILE_SHARE_READ,
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,  // sequential hint
    nullptr
);

LARGE_INTEGER file_size{};
::GetFileSizeEx(file_handle_, &file_size);
size_ = static_cast<size_t>(file_size.QuadPart);

mapping_handle_ = ::CreateFileMappingW(
    file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr
);

data_ = ::MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0);

// Teardown — three handles, in reverse order
::UnmapViewOfFile(data_);         // no size argument needed
::CloseHandle(mapping_handle_);
::CloseHandle(file_handle_);
```

### Platform Comparison

| Detail | POSIX | Windows |
|---|---|---|
| File handle type | `int` (fd) | `HANDLE` |
| Invalid handle sentinel | `-1` | `INVALID_HANDLE_VALUE` |
| Mapping object | Implicit in `mmap` | Explicit `HANDLE` from `CreateFileMapping` |
| Unmap | `munmap(ptr, size)` | `UnmapViewOfFile(ptr)` — no size needed |
| Sequential hint | `madvise(..., MADV_SEQUENTIAL)` | `FILE_FLAG_SEQUENTIAL_SCAN` at open time |
| Empty file | `mmap` fails on size 0 — guard required | `CreateFileMapping` fails on size 0 — guard required |
| Unicode paths | UTF-8 natively via `const char*` | Requires `CreateFileW` + `std::wstring` |
| Headers required | `<fcntl.h>`, `<sys/mman.h>`, `<sys/stat.h>`, `<unistd.h>` | `<windows.h>` |

### Third-Party Alternative

Boost.Iostreams (`boost::iostreams::mapped_file_source`) abstracts the platform split entirely. It is the main third-party option when Boost is already a project dependency.

---

## minimp3 Integration

minimp3 is a single-header C library. The C++ layer wraps it using RAII and modern types to eliminate raw pointer arithmetic and manual memory management.

### C vs Modern C++ Approach

| C Approach | Modern C++ Equivalent |
|---|---|
| `malloc` / `free` | `std::vector<uint8_t>` (RAII) |
| Raw `uint8_t*` + `size_t` | `std::span<const uint8_t>` |
| Manual pointer arithmetic | `span::subspan()` |
| `fopen` / `fread` / `fclose` | `std::ifstream` or `MappedFile` (RAII) |
| Output array on stack | `std::vector<int16_t>` resized to actual output |
| Global / manual state | Encapsulated in `Mp3Decoder` class |
| Error codes ignored | `std::runtime_error` thrown |

### Decoder Class

```cpp
class Mp3Decoder
{
public:
    Mp3Decoder() { mp3dec_init(&decoder_); }

    Mp3Decoder(const Mp3Decoder&)            = delete;
    Mp3Decoder& operator=(const Mp3Decoder&) = delete;

    std::vector<int16_t> decode_frame(
        std::span<const uint8_t> input,
        Mp3FrameInfo&            out_info,
        size_t&                  consumed);

private:
    mp3dec_t decoder_{};
};
```

### Decode Pipeline (Zero Copy)

```cpp
MappedFile file("audio.mp3");
Mp3Decoder decoder;

auto remaining = file.bytes();   // std::span — no allocation, points into mmap'd pages
Mp3FrameInfo info{};

while (!remaining.empty())
{
    size_t consumed = 0;
    auto   frame    = decoder.decode_frame(remaining, info, consumed);
    if (consumed == 0) break;
    remaining = remaining.subspan(consumed);  // advance — no pointer arithmetic
}
```

### MP3 Frame Decoding Internals

minimp3 processes one frame at a time. Each MPEG Layer III frame yields exactly 1152 PCM samples.

```
Raw bytes (mmap'd)
      │
      ▼
┌─────────────────────────────────────┐
│  1. Sync word scan (0xFFFB)         │  ← Locate frame boundaries
│  2. Parse frame header (4 bytes)    │  ← Bitrate, sample rate, channels
│  3. Read frame payload              │  ← Variable length (bitrate-dependent)
│  4. Huffman decode                  │  ← Compressed frequency coefficients
│  5. Requantise & dequantise         │  ← Restore amplitude scale
│  6. IMDCT                           │  ← Frequency domain → time domain
│  7. Polyphase synthesis filterbank  │  ← Final PCM output
└─────────────────────────────────────┘
      │
      ▼
 int16_t PCM samples (typically 44100 Hz, stereo)
```

---

## `is_supported` on a Filename

Exposing `is_supported(path)` on `MediaAnalyser` — rather than only on `Mp3Decoder` — is justified for CLI tools:

```cpp
if (!analyser.is_supported(argv[1])) {
    std::cerr << "Error: not a supported file.\n";
    return 1;
}
```

It catches all exceptions internally and returns `false` for both unreadable files and unsupported formats. This is deliberate — CLI validation should not crash, but `analyse()` lets exceptions propagate for precise diagnostics.

---

## C++ Version Requirements

| Feature | Minimum Standard |
|---|---|
| `std::filesystem::path` | C++17 |
| `std::span<const uint8_t>` | C++20 |
| `[[nodiscard]]` | C++17 |
| NRVO / move semantics | C++11 |
| `std::vector`, `std::ifstream` | C++11 |

If targeting C++17 only, replace `std::span` with a `{const uint8_t*, size_t}` struct (`BufferView`) — the decoder and analyser interfaces require no other changes.

---

## C++17 → C++20 Migration Path

| C++17 | C++20 |
|---|---|
| `struct BufferView { const std::byte* data; size_t size; }` | `using BufferView = std::span<const std::byte>` |
| Manual `MappedFile` RAII | Unchanged — no standard alternative yet |
| `std::filesystem::path` | Unchanged |

The decoder and analyser interfaces require no changes when migrating to C++20.
