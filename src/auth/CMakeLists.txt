# src/auth/CMakeLists.txt
add_library(auth_lib
    role_manager.cpp
    user_manager.cpp
    auth_service.cpp
    permission_checker.cpp
    user_storage_manager.cpp
)

target_include_directories(auth_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(auth_lib util_lib storage_lib catalog)

# 测试程序
add_executable(auth_test auth_test.cpp)
target_link_libraries(auth_test auth_lib)


# 使用示例
add_executable(auth_example usage_example.cpp)
target_link_libraries(auth_example auth_lib)
