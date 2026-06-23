cmake_minimum_required(VERSION 3.20)

add_library(poker_core STATIC
    src/deck.cpp
    src/hand_evaluator.cpp
    src/protocol.cpp
    src/packet_framing.cpp
    src/side_pots.cpp
)

target_include_directories(poker_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(poker_core PUBLIC third_party)
