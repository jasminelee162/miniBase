cmake_minimum_required(VERSION 3.10)

# Engine: executor library
add_library(executor_lib STATIC
    executor/Executor.cpp
)

target_include_directories(executor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(executor_lib PUBLIC
    storage_lib
    util_lib
    auth_lib
)
