add_library(util_lib STATIC
    logger.cpp
    config.cpp
)
target_include_directories(util_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

