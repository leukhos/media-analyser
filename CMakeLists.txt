cmake_minimum_required(VERSION 3.30)
project(media-analyser)


# Main library
add_library(media-analyser STATIC)
add_library(media_analyser::media_analyser ALIAS media-analyser)

target_sources(media-analyser
    PUBLIC
        FILE_SET media_analyser_headers
        TYPE HEADERS
        BASE_DIRS include
        FILES
            include/logger.hpp
            include/media_analyser.hpp
            include/media_decoder.hpp
            include/media_types.hpp
            include/file_loader.hpp
    PRIVATE
        src/logger.cpp
        src/media_analyser.cpp
        src/media_decoder.cpp
        src/file_loader.cpp
)

target_compile_features(media-analyser PUBLIC cxx_std_17)

add_executable(mp3task)

target_sources(mp3task
    PRIVATE
        src/main.cpp
)

target_link_libraries(mp3task
    PRIVATE
        media-analyser
)

install(
    TARGETS media-analyser
    EXPORT media_analyser_targets
    FILE_SET media_analyser_headers
)

install(
    EXPORT media_analyser_targets
    DESTINATION share/cmake/media-analyser
    NAMESPACE media_analyser::
)

install(
    FILES cmake/calculatorConfig.cmake
    DESTINATION share/cmake/media-analyser
)

include(CPack)

option(MEDIA_ANALYSER_ENABLE_TEST "Enable testing" OFF)
if(MEDIA_ANALYSER_ENABLE_TEST)
    find_package(doctest REQUIRED)
    find_package(mp3lame CONFIG REQUIRED)
    include(CTest)
    include(doctest)
    target_compile_definitions(media-analyser PRIVATE MEDIA_ANALYSER_TEST)

    add_executable(media-analyser-tests)
    target_sources(media-analyser-tests
        PRIVATE
            tests/main.cpp
            tests/unit/logger.utest.cpp
            tests/unit/media_decoder.utest.cpp
            tests/integration/file_loader.itest.cpp
    )
    target_link_libraries(media-analyser-tests
        PRIVATE
            media-analyser
            doctest::doctest
            mp3lame::mp3lame
    )
    doctest_discover_tests(media-analyser-tests)
endif()


# option(MEDIA_ANALYSER_ENABLE_BENCH "Enable benchmarking" OFF)
# if(MEDIA_ANALYSER_ENABLE_BENCH)
#     find_package(benchmark REQUIRED)

#     add_executable(media-analyser-bench)

#     target_sources(media-analyser-bench
#         PRIVATE
#             benches/calculator.benchmark.cpp
#     )

#     target_link_libraries(media-analyser-bench
#         PRIVATE
#             calculator
#             benchmark::benchmark
#     )

# endif()
