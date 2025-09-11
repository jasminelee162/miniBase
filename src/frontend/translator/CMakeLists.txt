cmake_minimum_required(VERSION 3.10)

add_library(translator_lib STATIC
    json_to_plan.cpp
)

target_include_directories(translator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(translator_lib PUBLIC
    util_lib
)

target_include_directories(translator_lib PUBLIC ${CMAKE_SOURCE_DIR}/src/engine/operators)
