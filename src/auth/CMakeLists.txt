# src/auth/CMakeLists.txt
add_library(auth_lib
    role_manager.cpp
    auth_service.cpp
    permission_checker.cpp
    user_storage_manager.cpp
)

target_include_directories(auth_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(auth_lib util_lib storage_lib catalog_lib)

# 使用示例已移动到 tests/unit/ 目录
