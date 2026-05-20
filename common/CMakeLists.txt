add_library(common_lib STATIC
    src/card.cpp
    src/deck.cpp
    src/evaluator.cpp
)

target_include_directories(common_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)