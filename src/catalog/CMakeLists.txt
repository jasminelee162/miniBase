# Catalog模块的CMakeLists.txt

# # 添加Catalog库（只有头文件，不需要编译源文件）
# add_library(catalog INTERFACE)

# # 设置包含目录
# target_include_directories(catalog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# 添加Catalog库（实现文件在 catalog.cpp ）
add_library(catalog_lib STATIC
	catalog.cpp
	catalog.h
)

# 设置包含目录并对外可见
target_include_directories(catalog_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 导出符号（Windows 下可选）
if(MSVC)
	target_compile_definitions(catalog_lib PRIVATE "CATALOG_EXPORTS")
endif()