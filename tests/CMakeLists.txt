add_library(Catch2 STATIC 
    external/catch_amalgamated.cpp
    external/catch_amalgamated.hpp
)

target_include_directories(Catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external)

add_executable(poker_tests 
    test_deck.cpp
)

target_link_libraries(poker_tests PRIVATE 
    poker_core
    Catch2
)

add_test(NAME PokerCoreTests COMMAND poker_tests)