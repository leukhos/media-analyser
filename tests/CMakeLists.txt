find_package(doctest CONFIG REQUIRED)
find_package(mp3lame CONFIG REQUIRED)
find_package(trompeloeil CONFIG REQUIRED)
include(doctest)

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

doctest_discover_tests(media_analyser_unit_tests)

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

doctest_discover_tests(media_analyser_integration_tests)