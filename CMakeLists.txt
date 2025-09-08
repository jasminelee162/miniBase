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
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/W4 /Zi /Od /utf-8)
    else()
        add_compile_options(/W4 /O2 /DNDEBUG /utf-8)
    endif()
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

