cmake_minimum_required(VERSION 3.20)
project(poker_client VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# add_library(poker_client_lib STATIC
#     # add other .cpp
# )

# target_include_directories(poker_client_lib PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# target_compile_options(poker_client_lib PRIVATE -Wall -Wextra -Wpedantic)

# add_executable(poker_client src/main.cpp)
# target_link_libraries(poker_client PRIVATE poker_client_lib)
# target_link_libraries(poker_client PRIVATE poker_core)
# target_include_directories(poker_client PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include
# )

add_library(poker_client_lib INTERFACE)

target_include_directories(poker_client_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(poker_client_lib INTERFACE -Wall -Wextra -Wpedantic)

add_executable(poker_client src/main.cpp)
target_link_libraries(poker_client PRIVATE poker_client_lib)
target_link_libraries(poker_client PRIVATE poker_core)

target_include_directories(poker_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)