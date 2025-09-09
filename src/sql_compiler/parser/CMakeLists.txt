# 语法分析器的CMakeLists.txt

# 添加语法分析器库
add_library(parser
    ast.cpp
    ast.h
    parser.cpp
    parser.h
    ast_printer.cpp
    ast_printer.h
)

# 链接词法分析器库
target_link_libraries(parser
    lexer
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