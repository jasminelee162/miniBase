cmake_minimum_required(VERSION 3.15)

add_library(httplib_header INTERFACE)
target_include_directories(httplib_header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(minidb_webui
    server_main.cpp
)

target_link_libraries(minidb_webui
    httplib_header
    util_lib
    storage_lib
    catalog_lib
    auth_lib
    translator_lib
    lexer
    parser
    semantic
    executor_lib
)

target_include_directories(minidb_webui PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link tools (sql dumper/importer)
target_sources(minidb_webui PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tools/sql_dump/sql_dumper.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/sql_import/sql_importer.cpp
)

# Copy static assets next to the binary so runtime can find them
add_custom_command(TARGET minidb_webui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/index.html
            $<TARGET_FILE_DIR:minidb_webui>/index.html
)


