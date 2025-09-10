# Catalog模块的CMakeLists.txt

# 添加Catalog库（只有头文件，不需要编译源文件）
add_library(catalog INTERFACE)

# 设置包含目录
target_include_directories(catalog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})