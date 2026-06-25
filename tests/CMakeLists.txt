add_library(Catch2 STATIC
    external/catch_amalgamated.cpp
    external/catch_amalgamated.hpp
)

target_include_directories(Catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external)

add_executable(poker_tests
    test_deck.cpp
    test_hand_evaluator.cpp
    test_protocol.cpp
    test_packet_framing.cpp
    test_side_pots.cpp
    test_game_session.cpp
    test_lobby.cpp
    test_integration.cpp
    test_player_name_registry.cpp
    test_user_repository.cpp
    test_login_auth.cpp
    test_auth.cpp
    test_database.cpp
    test_chip_persistence.cpp
)

target_include_directories(poker_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/client/include
    ${CMAKE_SOURCE_DIR}/server/include
    ${CMAKE_SOURCE_DIR}/tests/support
)

target_link_libraries(poker_tests PRIVATE
    poker_core
    poker_server_lib
    poker_client_lib
    Catch2
)

add_test(NAME UnitTests COMMAND poker_tests "~[integration]")
add_test(NAME IntegrationTests COMMAND poker_tests "[integration]")
