# src目录的CMakeLists.txt

# 添加子目录
# add_subdirectory(catalog)
add_subdirectory(sql_compiler/lexer)
add_subdirectory(sql_compiler/parser)
add_subdirectory(sql_compiler/semantic)
add_subdirectory(sql_compiler/planner)
# add_subdirectory(engine)

# 未来可以添加其他模块
# add_subdirectory(engine/executor)
# add_subdirectory(engine/operators)
