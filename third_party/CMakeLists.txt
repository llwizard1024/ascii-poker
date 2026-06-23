add_library(third_party INTERFACE)
target_include_directories(third_party INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ftxui STATIC ftxui/ftxui.cpp)
target_include_directories(ftxui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ftxui)
target_link_libraries(ftxui PUBLIC pthread)
target_compile_options(ftxui PRIVATE -w)
