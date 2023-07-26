cmake_minimum_required(VERSION 3.20)

project(SmoothLife LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}-Binaries")

add_executable(SmoothLifeGPU smoothlife_gpu.c)

find_package(raylib CONFIG REQUIRED)
target_link_libraries(SmoothLifeGPU PRIVATE raylib)

# copy shader
add_custom_command(
    TARGET SmoothLifeGPU POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_hardlink
    ${CMAKE_CURRENT_SOURCE_DIR}/smoothlife.fs
    $<TARGET_FILE_DIR:SmoothLifeGPU>/smoothlife.fs
)