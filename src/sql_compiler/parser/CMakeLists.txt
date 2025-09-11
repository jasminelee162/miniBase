# 语法分析器的CMakeLists.txt

# 添加语法分析器库
add_library(parser
    ast.cpp
    ast.h
    parser.cpp
    parser.h
    ast_printer.cpp
    ast_printer.h
    ast_json_serializer.cpp
    ast_json_serializer.h
)

# 链接词法分析器库
target_link_libraries(parser
    lexer
)

# Link util for Logger
target_link_libraries(parser
    util_lib
)

# 添加语法分析器测试可执行文件
add_executable(parser_test
    parser_test.cpp
)

# 链接语法分析器库到测试可执行文件
target_link_libraries(parser_test
    parser
    lexer
)

# parser_export 可执行文件 - 支持完整的编译到执行流程
add_executable(parser_export
    parser_export.cpp
)

# 链接所有必要的库以支持完整流程
target_link_libraries(parser_export
    parser
    lexer
    util_lib
    storage_lib
    executor_lib
    translator_lib
    catalog_lib
)

target_link_libraries(parser_test
    util_lib
)