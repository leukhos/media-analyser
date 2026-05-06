# TODO

## CMake structure improvements

### High priority

- [ ] Align the CMake C++ standard with the project language level.
    - The project is intended to use C++20.
    - Update target compile features from C++17 to C++20.

- [ ] Move `include(CPack)` to the end of the root CMake configuration.
    - CPack should generally be included after install rules and package metadata are configured.

- [ ] Install the library target and public headers.
    - The current install setup only installs the executable.
    - If the library is part of the public API, install:
        - executable runtime
        - library/archive artifacts
        - public header file set

### Medium priority

- [ ] Add CMake package export support.
    - Add `install(EXPORT ...)`.
    - Add a namespace for exported targets.
    - Consider generating:
        - package config file
        - package version file
    - This will allow external CMake projects to consume the installed package.

- [ ] Set a clean exported target name for the library.
    - Add an `EXPORT_NAME` property so consumers can use a clean target name such as:
        - `media_analyser::media_analyser`

- [ ] Either remove or implement benchmark support.
    - A benchmark preset exists.
    - A benchmark vcpkg feature exists.
    - There is currently no benchmark option/subdirectory wired into CMake.
    - Either:
        - remove the benchmark preset for now, or
        - add `MEDIA_ANALYSER_ENABLE_BENCH` and a benchmark subdirectory.

### Low priority

- [ ] Reconsider forcing compilers in presets.
    - Current platform presets explicitly select compilers.
    - This is acceptable for controlled environments, but less flexible for:
        - custom compiler selection
        - compiler wrappers
        - cross-compilation
        - user-provided toolchains
    - Consider providing generic presets and separate explicit compiler presets.

- [ ] Move warning flags from global preset variables to target-based CMake configuration.
    - Prefer `target_compile_options(...)` or dedicated interface targets.
    - This avoids leaking warning options globally and scales better.

- [ ] Consider splitting unit and integration tests.
    - Current test dependencies include codec-related libraries.
    - If the suite grows, separate:
        - pure unit tests
        - integration/media-codec tests
    - This can reduce required dependencies for quick unit test runs.

## Notes

- The overall CMake structure is clean and simple.
- The separation between root configuration, production targets, tests, presets, and vcpkg features is good.
- Modern CMake header file sets are already being used.
- Optional tests through `MEDIA_ANALYSER_ENABLE_TESTS` are a good design choice.