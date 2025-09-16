# Optimizer module
add_library(optimizer_lib STATIC
    index_optimizer.cpp
    index_optimizer.h
)

target_include_directories(optimizer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(optimizer_lib PUBLIC
    storage_lib
    catalog_lib
    util_lib
)
