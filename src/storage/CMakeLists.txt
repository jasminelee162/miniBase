# 缓冲池实现
add_subdirectory(buffer)

add_library(storage_lib STATIC
    page/disk_manager.cpp
    buffer/buffer_pool_manager.cpp
    index/bplus_tree.cpp
    storage_engine.cpp
)

target_link_libraries(storage_lib
    storage_buffer_lib
)

target_include_directories(storage_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)