# 语义分析器的CMakeLists.txt

# 添加语义分析器库
add_library(semantic
    semantic_analyzer.cpp
    semantic_analyzer.h
)

# 链接依赖库
target_link_libraries(semantic
    parser
    lexer
)

# 添加语义分析器测试可执行文件
add_executable(semantic_test
    semantic_test.cpp
)

# 链接语义分析器库到测试可执行文件
target_link_libraries(semantic_test
    semantic
    parser
    lexer
)