cmake_minimum_required(VERSION 3.20)
project(poker_server VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# add_library(poker_server_lib STATIC
#     # .cpp other
# )

# target_include_directories(poker_server_lib PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# target_compile_options(poker_server_lib PRIVATE -Wall -Wextra -Wpedantic)

# add_executable(poker_server src/main.cpp)
# target_link_libraries(poker_server PRIVATE poker_server_lib)
# target_link_libraries(poker_server PRIVATE poker_core)
# target_include_directories(poker_server PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include
# )

add_library(poker_server_lib INTERFACE)

target_include_directories(poker_server_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(poker_server_lib INTERFACE -Wall -Wextra -Wpedantic)

add_executable(poker_server src/main.cpp)
target_link_libraries(poker_server PRIVATE poker_server_lib)
target_link_libraries(poker_server PRIVATE poker_core)

target_include_directories(poker_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)