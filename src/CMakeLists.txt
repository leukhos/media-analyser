# Main library
add_library(media-analyser-lib)
add_library(media_analyser::media_analyser ALIAS media-analyser-lib)

target_sources(media-analyser-lib
        PUBLIC
        FILE_SET media_analyser_headers
        TYPE HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES
        ${PROJECT_SOURCE_DIR}/include/logger.hpp
        ${PROJECT_SOURCE_DIR}/include/media_analyser.hpp
        ${PROJECT_SOURCE_DIR}/include/media_decoder.hpp
        ${PROJECT_SOURCE_DIR}/include/media_types.hpp
        ${PROJECT_SOURCE_DIR}/include/file_loader.hpp
        PRIVATE
        logger.cpp
        media_analyser.cpp
        media_decoder.cpp
        file_loader.cpp
)

target_include_directories(media-analyser-lib
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(media-analyser-lib PUBLIC cxx_std_17)

add_executable(media-analyser)
target_sources(media-analyser
        PRIVATE
        main.cpp
)
target_compile_features(media-analyser PRIVATE cxx_std_17)
target_link_libraries(media-analyser
        PRIVATE
        media-analyser-lib
)

install(
        TARGETS media-analyser
        RUNTIME DESTINATION bin
)