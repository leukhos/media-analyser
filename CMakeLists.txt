cmake_minimum_required(VERSION 3.30)
project(media-analyser VERSION 1.0.0 LANGUAGES CXX)

option(MEDIA_ANALYSER_ENABLE_TESTS "Enable testing" OFF)

add_subdirectory(src)
include(CPack)

if (MEDIA_ANALYSER_ENABLE_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()
