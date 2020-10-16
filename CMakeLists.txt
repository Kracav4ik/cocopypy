cmake_minimum_required(VERSION 3.16)
project(cocopypy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(cocopypy
        main.cpp
)

target_link_libraries(cocopypy winmm.lib)

target_link_options(
        cocopypy PRIVATE -municode
)
