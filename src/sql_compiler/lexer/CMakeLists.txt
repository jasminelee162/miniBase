# 词法分析器的CMakeLists.txt

# 添加词法分析器库
add_library(lexer
    lexer.cpp
    lexer.h
    token.h
)

# 添加词法分析器测试可执行文件
add_executable(lexer_test
    lexer_test.cpp
)

# 链接词法分析器库到测试可执行文件
target_link_libraries(lexer_test
    lexer
)

# Ensure util library is linked so Logger symbols are resolved
target_link_libraries(lexer
    util_lib
)

target_link_libraries(lexer_test
    util_lib
)