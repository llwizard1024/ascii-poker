add_library(common_lib STATIC
    src/card.cpp
    src/deck.cpp
    src/evaluator.cpp
)

target_include_directories(common_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

enable_testing()

add_executable(common_lib_tests
    external/catch2/catch_amalgamated.cpp
    tests/test_evaluator.cpp
)

target_link_libraries(common_lib_tests PRIVATE common_lib)
target_include_directories(common_lib_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)