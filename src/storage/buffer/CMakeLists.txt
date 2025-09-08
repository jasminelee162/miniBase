add_library(storage_buffer_lib STATIC
    lru_replacer.cpp
)

target_include_directories(storage_buffer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

