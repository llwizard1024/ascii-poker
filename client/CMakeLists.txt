cmake_minimum_required(VERSION 3.16)
project(poker_client VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(poker_client_lib STATIC
    src/utils/helper.cpp
    # and other
)

target_include_directories(poker_client_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(poker_client_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(poker_client src/main.cpp)
target_link_libraries(poker_client PRIVATE poker_client_lib)
target_link_libraries(poker_client PRIVATE common_lib)
target_include_directories(poker_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

add_executable(poker_client_tests
    external/catch2/catch_amalgamated.cpp
    tests/test_utils.cpp
)

target_link_libraries(poker_client_tests PRIVATE poker_client_lib)
target_link_libraries(poker_client_tests PRIVATE common_lib)
target_include_directories(poker_client_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)