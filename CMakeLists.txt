cmake_minimum_required(VERSION 3.20)
project(ascii_poker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_subdirectory(third_party)
add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)
