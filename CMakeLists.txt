# ====================================================================

cmake_minimum_required(VERSION 2.8)

project(JpegWrapper)

# ====================================================================

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# ====================================================================

add_subdirectory(libjpeg)

# ====================================================================

include_directories(libjpeg/inc)

add_executable(test_gray01 test/test_gray01.c XJPEG_wrapper.c)
target_link_libraries(test_gray01 libjpeg)

add_executable(jerase test/jerase.c XJPEG_wrapper.c)
target_link_libraries(jerase libjpeg)

# ====================================================================
