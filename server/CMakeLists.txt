cmake_minimum_required(VERSION 3.16)
project(poker_server VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(poker_server_lib STATIC
    src/utils/helper.cpp
    # and other
)

target_include_directories(poker_server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(poker_server_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(poker_server src/main.cpp)
target_link_libraries(poker_server PRIVATE poker_server_lib)
target_link_libraries(poker_server PRIVATE common_lib)

enable_testing()

add_executable(poker_server_tests
    external/catch2/catch_amalgamated.cpp
    tests/test_utils.cpp
)

target_link_libraries(poker_server_tests PRIVATE poker_server_lib)
target_link_libraries(poker_server_tests PRIVATE common_lib)
target_include_directories(poker_server_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)