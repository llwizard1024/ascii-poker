cmake_minimum_required(VERSION 3.20)

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(poker_server_lib STATIC
    src/network/server.cpp
    src/network/session.cpp
    src/network/connection_registry.cpp
    src/network/player_name_registry.cpp
    src/messaging/lobby_message_processor.cpp
    src/messaging/handshake_message_processor.cpp
    src/game/room.cpp
    src/game/lobby.cpp
    src/game/remote_player.cpp
    src/game/game_session.cpp
    src/storage/database.cpp
    src/storage/password_hash.cpp
    src/storage/user_repository.cpp
)

target_include_directories(poker_server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(poker_server_lib PUBLIC poker_core third_party SQLite3::SQLite3 OpenSSL::Crypto)
target_compile_options(poker_server_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(poker_server src/main.cpp)
target_link_libraries(poker_server PRIVATE poker_server_lib)
