find_package(doctest CONFIG REQUIRED)
find_package(mp3lame CONFIG REQUIRED)
find_package(trompeloeil CONFIG REQUIRED)
include(doctest)

add_executable(media-analyser-tests)
target_sources(media-analyser-tests
        PRIVATE
        main.cpp
        unit/logger.utest.cpp
        unit/media_analyser.utest.cpp
        unit/media_decoder.utest.cpp
        integration/file_loader.itest.cpp
        integration/media_analyser.itest.cpp
)

target_link_libraries(media-analyser-tests
        PRIVATE
        media-analyser-lib
        doctest::doctest
        mp3lame::mp3lame
        trompeloeil::trompeloeil
)

doctest_discover_tests(media-analyser-tests)