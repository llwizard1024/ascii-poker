cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(poker_core STATIC
    src/card.cpp
    src/deck.cpp
    src/hand_evaluator.cpp
    src/protocol.cpp
)

target_include_directories(poker_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)