# src/CMakeLists.txt

# 工具库（基础依赖）
add_subdirectory(util)

# 存储系统（核心模块优先打通）
add_subdirectory(storage)

# 系统目录（Catalog）
add_subdirectory(catalog)

# 编译器模块
add_subdirectory(sql_compiler/lexer)
add_subdirectory(sql_compiler/parser)
add_subdirectory(sql_compiler/semantic)
add_subdirectory(sql_compiler/planner)
# Add engine and frontend translator directories so their libraries are built
add_subdirectory(engine)
add_subdirectory(frontend/translator)

# CLI
add_subdirectory(cli)

# 权限管理模块
add_subdirectory(auth)

# 临时仅链接 util 与 storage，其他模块待实现后再启用
add_library(minidb_core INTERFACE)
target_link_libraries(minidb_core INTERFACE
    util_lib
    storage_lib
)

# 主程序（最小示例）
add_executable(minidb main.cpp)
target_link_libraries(minidb minidb_core Threads::Threads)# src目录的CMakeLists.txt


