cmake_minimum_required(VERSION 3.20)

add_library(poker_client_lib STATIC
    src/network_client.cpp
)

target_include_directories(poker_client_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(poker_client_lib PUBLIC poker_core)
target_compile_options(poker_client_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(poker_client src/main.cpp)
target_link_libraries(poker_client PRIVATE poker_client_lib)