# 添加planner库
add_library(
    planner
    planner.cpp
    planner.h
    plan_printer.cpp
    plan_printer.h
)

# 设置包含目录
target_include_directories(planner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖库
target_link_libraries(planner PUBLIC parser semantic)

# 添加测试可执行文件
add_executable(planner_test planner_test.cpp)

# 链接依赖库
target_link_libraries(planner_test PUBLIC planner parser lexer semantic)