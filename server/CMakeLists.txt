cmake_minimum_required(VERSION 3.20)

add_library(poker_server_lib STATIC
    src/network/server.cpp
    src/network/session.cpp
    src/messaging/echo_message_processor.cpp
    src/messaging/lobby_message_processor.cpp
    src/game/room.cpp
    src/game/lobby.cpp
    src/game/remote_player.cpp
    src/game/game_session.cpp
)

target_include_directories(poker_server_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(poker_server_lib PUBLIC poker_core)
target_compile_options(poker_server_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(poker_server src/main.cpp)
target_link_libraries(poker_server PRIVATE poker_server_lib)