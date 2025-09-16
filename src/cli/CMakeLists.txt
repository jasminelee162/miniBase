add_library(minidb_cli_lib STATIC
    minidb/cli_helpers.cpp
    minidb/user_management.cpp
    minidb/command_handlers.cpp
    minidb/input_accumulator.cpp
    minidb/execution_pipeline.cpp
)

target_include_directories(minidb_cli_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/minidb
)

target_link_libraries(minidb_cli_lib
    PRIVATE
        lexer
        parser
        semantic
        translator_lib
        executor_lib
        storage_lib
        catalog_lib
        util_lib
        planner
        tools_lib
)

add_executable(minidb_cli
    minidb_cli.cpp
)

target_link_libraries(minidb_cli
    PRIVATE
        minidb_cli_lib
)

target_include_directories(minidb_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/minidb
)