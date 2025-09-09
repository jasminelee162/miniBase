# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
# Prefer extraction timestamp for URL downloads (avoids unnecessary rebuilds)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(MiniDB VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项（区分 MSVC 与非 MSVC）
if(MSVC)
    # VS 多配置生成器下不要依赖 CMAKE_BUILD_TYPE，使用按配置选项
    add_compile_options(/W4 /utf-8)
    # 分开写，避免生成器表达式被当作单一参数解析异常
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
    add_compile_options($<$<CONFIG:Debug>:/Od>)
    add_compile_options($<$<CONFIG:Debug>:/RTC1>)
    add_compile_options($<$<CONFIG:Release>:/O2>)
    add_compile_options($<$<CONFIG:Release>:/DNDEBUG>)
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -fsanitize=address")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
    endif()
endif()

# 包含目录
include_directories(src)

# 提供项目根目录给代码使用（用于固定日志/数据文件路径）
add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

# 查找依赖库
find_package(Threads REQUIRED)

# 可选：测试框架
option(BUILD_TESTS "Build tests" ON)

# 添加库和可执行文件
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

