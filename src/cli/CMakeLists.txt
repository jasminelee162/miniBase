add_executable(minidb_cli
    minidb_cli.cpp
)

target_link_libraries(minidb_cli
    lexer
    parser
    semantic
    translator_lib
    executor_lib
    storage_lib
    catalog
    util_lib
) 