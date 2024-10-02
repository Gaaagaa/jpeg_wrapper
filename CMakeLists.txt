# ====================================================================

cmake_minimum_required(VERSION 2.8)

project(JpegWrapper)

# ====================================================================

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# ====================================================================

add_subdirectory(libjpeg)

# ====================================================================

include_directories(
                libjpeg/include
                src)

add_executable(jclip test/jclip.cpp src/jdecoder.c src/jencoder.c)
target_link_libraries(jclip libjpeg)

# ====================================================================
