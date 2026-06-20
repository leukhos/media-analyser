include(FetchContent)

FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest
        GIT_TAG 6804767ee637789db8a5cb281381cae98dc36906 # v2.5.2
        FIND_PACKAGE_ARGS
)

FetchContent_Declare(
        trompeloeil
        GIT_REPOSITORY https://github.com/rollbear/trompeloeil
        GIT_TAG v49
        FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(doctest trompeloeil)

find_package(mp3lame REQUIRED)

add_executable(media_analyser_unit_tests)
target_sources(media_analyser_unit_tests
        PRIVATE
        unit/main.cpp
        unit/logger.utest.cpp
        unit/media_analyser.utest.cpp
        unit/media_decoder.utest.cpp
)

media_analyser_enable_warnings(media_analyser_unit_tests)

target_link_libraries(media_analyser_unit_tests
        PRIVATE
        media-analyser-lib
        doctest::doctest
        mp3lame::mp3lame
        trompeloeil::trompeloeil
)

add_executable(media_analyser_integration_tests)
target_sources(media_analyser_integration_tests
        PRIVATE
        integration/main.cpp
        integration/file_loader.itest.cpp
        integration/media_analyser.itest.cpp
)

media_analyser_enable_warnings(media_analyser_integration_tests)

target_link_libraries(media_analyser_integration_tests
        PRIVATE
        media-analyser-lib
        doctest::doctest
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(media_analyser_unit_tests)
doctest_discover_tests(media_analyser_integration_tests)
