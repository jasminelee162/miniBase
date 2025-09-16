# 简单的测试配置，不使用GoogleTest
# 统一可执行文件输出目录，便于 CTest 定位（多配置生成器下设置 Debug 路径）
if (MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests/Debug)
endif()
# 每个测试可执行文件只能包含一个 main，因此拆分为独立目标

# 1) storage_test
add_executable(storage_test
    unit/storage_test.cpp
    simple_test_framework.cpp
)

# Use gtest_main provided by fetchcontent
find_package(Threads REQUIRED)

target_link_libraries(storage_test
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME storage_test COMMAND storage_test)

# 将测试的工作目录设置为项目根目录，便于在根目录生成 .db/.log
set_tests_properties(storage_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# End-to-end flow test: parser -> json -> translator -> executor
add_executable(e2e_flow
    unit/e2e_flow.cpp
)

target_link_libraries(e2e_flow
    parser
    lexer
    util_lib
    storage_lib
)
target_link_libraries(e2e_flow
    semantic
)
target_link_libraries(e2e_flow
    translator_lib
    executor_lib
)

add_test(NAME e2e_flow COMMAND e2e_flow)
set_tests_properties(e2e_flow PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# 2) storage_size_test（从 storage_test 中拆出，避免多个 main 冲突）
add_executable(storage_size_test
    unit/storage_size_test.cpp
    simple_test_framework.cpp
)

target_link_libraries(storage_size_test
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME storage_size_test COMMAND storage_size_test)

set_tests_properties(storage_size_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 3) page_linking_test（页链接专用用例）
add_executable(page_linking_test
    unit/page_linking_test.cpp
    simple_test_framework.cpp
)

target_link_libraries(page_linking_test
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME page_linking_test COMMAND page_linking_test)

set_tests_properties(page_linking_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 4) compiler_errors_test：覆盖lexer/parser/semantic错误路径
add_executable(compiler_errors_test
    unit/compiler_errors_test.cpp
)

target_link_libraries(compiler_errors_test
    lexer
    parser
    semantic
    util_lib
)

add_test(NAME compiler_errors_test COMMAND compiler_errors_test)
set_tests_properties(compiler_errors_test PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# 5) bplus_tree_test（最小版 B+Tree 索引用例）
add_executable(bplus_tree_test
    unit/bplus_tree_test.cpp
    simple_test_framework.cpp
)

target_link_libraries(bplus_tree_test
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME bplus_tree_test COMMAND bplus_tree_test)
set_tests_properties(bplus_tree_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 6) test_storage_enhancements（Storage模块增强功能测试）
add_executable(test_storage_enhancements
    unit/test_storage_enhancements.cpp
)

target_link_libraries(test_storage_enhancements
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME test_storage_enhancements COMMAND test_storage_enhancements)
set_tests_properties(test_storage_enhancements PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 7) test_bplus_tree_enhancements（B+树增强功能测试）
add_executable(test_bplus_tree_enhancements
    unit/test_bplus_tree_enhancements.cpp
)

target_link_libraries(test_bplus_tree_enhancements
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME test_bplus_tree_enhancements COMMAND test_bplus_tree_enhancements)
set_tests_properties(test_bplus_tree_enhancements PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# 8) page_types_api_test（四类页接口连通性测试）
add_executable(page_types_api_test
    unit/page_types_api_test.cpp
    simple_test_framework.cpp
)

target_link_libraries(page_types_api_test
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME page_types_api_test COMMAND page_types_api_test)
set_tests_properties(page_types_api_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# 9) bplus_tree_split_stress_test（多层分裂压力用例）
add_executable(bplus_tree_split_stress_test
    unit/bplus_tree_split_stress_test.cpp
    simple_test_framework.cpp
)

target_link_libraries(bplus_tree_split_stress_test
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME bplus_tree_split_stress_test COMMAND bplus_tree_split_stress_test)
set_tests_properties(bplus_tree_split_stress_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# 10) storage_end_to_end_demo（.bin创建→meta→catalog→data读写演示）
add_executable(storage_end_to_end_demo
    unit/storage_end_to_end_demo.cpp
    simple_test_framework.cpp
)

target_link_libraries(storage_end_to_end_demo
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME storage_end_to_end_demo COMMAND storage_end_to_end_demo)
set_tests_properties(storage_end_to_end_demo PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# 11) cli_test（CLI功能测试）
add_executable(cli_test
    unit/cli_test.cpp
    simple_test_framework.cpp
)

target_link_libraries(cli_test
    lexer
    parser
    semantic
    translator_lib
    executor_lib
    storage_lib
    catalog
    util_lib
    Threads::Threads
)

add_test(NAME cli_test COMMAND cli_test)
set_tests_properties(cli_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 12) executor_test（Executor功能测试）
add_executable(executor_test
    unit/executor_test.cpp
)

target_link_libraries(executor_test
    executor_lib
    storage_lib
    catalog
    util_lib
    Threads::Threads
)

add_test(NAME executor_test COMMAND executor_test)
set_tests_properties(executor_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
# 11) test_permissions_updated（权限管理测试）
add_executable(test_permissions_updated
    unit/test_permissions_updated.cpp
)

target_link_libraries(test_permissions_updated
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_permissions_updated COMMAND test_permissions_updated)
set_tests_properties(test_permissions_updated PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 12) test_auth_storage_integration（权限管理存储集成测试）
add_executable(test_auth_storage_integration
    unit/test_auth_storage_integration.cpp
)

target_link_libraries(test_auth_storage_integration
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_auth_storage_integration COMMAND test_auth_storage_integration)
set_tests_properties(test_auth_storage_integration PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 13) test_auth_bin_structure（权限管理.bin文件结构测试）
add_executable(test_auth_bin_structure
    unit/test_auth_bin_structure.cpp
)

target_link_libraries(test_auth_bin_structure
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_auth_bin_structure COMMAND test_auth_bin_structure)
set_tests_properties(test_auth_bin_structure PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 14) test_user_persistence_debug（用户数据持久化调试测试）
add_executable(test_user_persistence_debug
    unit/test_user_persistence_debug.cpp
)

target_link_libraries(test_user_persistence_debug
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_user_persistence_debug COMMAND test_user_persistence_debug)
set_tests_properties(test_user_persistence_debug PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 15) test_user_workflow（用户创建和登录流程测试）
add_executable(test_user_workflow
    unit/test_user_workflow.cpp
)

target_link_libraries(test_user_workflow
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_user_workflow COMMAND test_user_workflow)
set_tests_properties(test_user_workflow PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 16) test_simple（简单测试程序）
add_executable(test_simple
    unit/test_simple.cpp
)

target_link_libraries(test_simple
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_simple COMMAND test_simple)
set_tests_properties(test_simple PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 17) test_debug（调试测试程序）
add_executable(test_debug
    unit/test_debug.cpp
)

target_link_libraries(test_debug
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_debug COMMAND test_debug)
set_tests_properties(test_debug PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 18) test_cli（命令行测试程序）
add_executable(test_cli
    unit/test_cli.cpp
)

target_link_libraries(test_cli
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_cli COMMAND test_cli)
set_tests_properties(test_cli PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 19) test_auth_interfaces（权限管理接口完整测试）
add_executable(test_auth_interfaces
    unit/test_auth_interfaces.cpp
)

target_link_libraries(test_auth_interfaces
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_auth_interfaces COMMAND test_auth_interfaces)
set_tests_properties(test_auth_interfaces PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 20) test_auth_security（权限管理安全约束测试）
add_executable(test_auth_security
    unit/test_auth_security.cpp
)

target_link_libraries(test_auth_security
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME test_auth_security COMMAND test_auth_security)
set_tests_properties(test_auth_security PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 21) auth_test（权限管理基础测试）
add_executable(auth_test
    unit/auth_test.cpp
)

target_link_libraries(auth_test
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME auth_test COMMAND auth_test)
set_tests_properties(auth_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 22) auth_storage_test（权限管理存储测试）
add_executable(auth_storage_test
    unit/auth_storage_test.cpp
)

target_link_libraries(auth_storage_test
    auth_lib
    storage_lib
    catalog
    util_lib
)

add_test(NAME auth_storage_test COMMAND auth_storage_test)
set_tests_properties(auth_storage_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 20) minidb_auth_cli（认证版CLI演示程序）
add_executable(minidb_auth_cli
    unit/minidb_auth_cli.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/AuthCLI.cpp
)
target_link_libraries(minidb_auth_cli
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(minidb_auth_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 21) test_table_permissions（表级别权限控制测试）
add_executable(test_table_permissions
    unit/test_table_permissions.cpp
)
target_link_libraries(test_table_permissions
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_table_permissions PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 22) test_debug_permissions（权限调试程序）
add_executable(test_debug_permissions
    unit/test_debug_permissions.cpp
)
target_link_libraries(test_debug_permissions
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_debug_permissions PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 23) test_debug_role（角色调试程序）
add_executable(test_debug_role
    unit/test_debug_role.cpp
)
target_link_libraries(test_debug_role
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_debug_role PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 24) test_debug_set（set调试程序）
add_executable(test_debug_set
    unit/test_debug_set.cpp
)
target_link_libraries(test_debug_set
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_debug_set PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 25) test_simple_set（简单set测试）
add_executable(test_simple_set
    unit/test_simple_set.cpp
)
set_target_properties(test_simple_set PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 26) test_debug_map（map调试程序）
add_executable(test_debug_map
    unit/test_debug_map.cpp
)
target_link_libraries(test_debug_map
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_debug_map PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 27) test_permission_compare（Permission比较测试）
add_executable(test_permission_compare
    unit/test_permission_compare.cpp
)
target_link_libraries(test_permission_compare
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_permission_compare PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 28) test_role_manager_simple（RoleManager简单测试）
add_executable(test_role_manager_simple
    unit/test_role_manager_simple.cpp
)

target_link_libraries(test_role_manager_simple
    auth_lib
    storage_lib
    catalog
    util_lib
)
set_target_properties(test_role_manager_simple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 29) test_users_table_visibility（__users__ 可见性测试）
add_executable(test_users_table_visibility
    unit/test_users_table_visibility.cpp
    simple_test_framework.cpp
)

target_link_libraries(test_users_table_visibility
    auth_lib
    storage_lib
    catalog_lib
    util_lib
)

add_test(NAME test_users_table_visibility COMMAND test_users_table_visibility)
set_tests_properties(test_users_table_visibility PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 30) test_runtime_config_and_metrics（配置中心与指标基础测试）
add_executable(test_runtime_config_and_metrics
    unit/test_runtime_config_and_metrics.cpp
    simple_test_framework.cpp
)

target_link_libraries(test_runtime_config_and_metrics
    storage_lib
    util_lib
    Threads::Threads
)

add_test(NAME test_runtime_config_and_metrics COMMAND test_runtime_config_and_metrics)
set_tests_properties(test_runtime_config_and_metrics PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 统一：将所有测试目标加入一个聚合构建目标，便于一次性构建
set(ALL_TEST_TARGETS
    storage_test
    e2e_flow
    storage_size_test
    page_linking_test
    compiler_errors_test
    bplus_tree_test
    test_storage_enhancements
    test_bplus_tree_enhancements
    page_types_api_test
    bplus_tree_split_stress_test
    storage_end_to_end_demo
    cli_test
    executor_test
    test_permissions_updated
    test_auth_storage_integration
    test_auth_bin_structure
    test_user_persistence_debug
    test_user_workflow
    test_simple
    test_debug
    test_cli
    test_auth_interfaces
    test_auth_security
    auth_test
    auth_storage_test
    test_users_table_visibility
    test_runtime_config_and_metrics
    test_page_type_validation
    test_meta_non_overwrite
    test_constraint_validation
    test_concurrency_correctness
    bench_storage_rw
)

add_custom_target(tests_all DEPENDS ${ALL_TEST_TARGETS})

# 多配置下显式设置输出目录，避免 CTest 找不到二进制
foreach(tgt ${ALL_TEST_TARGETS})
    if (TARGET ${tgt})
        set_target_properties(${tgt} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests/Debug
        )
    endif()
endforeach()

# 如果存在 RUN_TESTS 目标，让其依赖于 tests_all，确保先构建再运行
if (TARGET RUN_TESTS)
    add_dependencies(RUN_TESTS tests_all)
endif()

# 31) test_page_type_validation（页类型校验）
add_executable(test_page_type_validation
    unit/test_page_type_validation.cpp
    simple_test_framework.cpp
)
target_link_libraries(test_page_type_validation
    storage_lib
    util_lib
    Threads::Threads
)
add_test(NAME test_page_type_validation COMMAND test_page_type_validation)
set_tests_properties(test_page_type_validation PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# 32) test_meta_non_overwrite（Meta 不覆盖校验）
add_executable(test_meta_non_overwrite
    unit/test_meta_non_overwrite.cpp
    simple_test_framework.cpp
)
target_link_libraries(test_meta_non_overwrite
    storage_lib
    util_lib
    Threads::Threads
)
add_test(NAME test_meta_non_overwrite COMMAND test_meta_non_overwrite)
set_tests_properties(test_meta_non_overwrite PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# 33) test_constraint_validation（约束校验）
add_executable(test_constraint_validation
    unit/test_constraint_validation.cpp
    simple_test_framework.cpp
)
target_link_libraries(test_constraint_validation
    executor_lib
    translator_lib
    parser
    lexer
    semantic
    storage_lib
    catalog_lib
    auth_lib
    util_lib
    Threads::Threads
)
add_test(NAME test_constraint_validation COMMAND test_constraint_validation)
set_tests_properties(test_constraint_validation PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# 34) test_concurrency_correctness（并发正确性）
add_executable(test_concurrency_correctness
    unit/test_concurrency_correctness.cpp
    simple_test_framework.cpp
)
target_link_libraries(test_concurrency_correctness
    storage_lib
    util_lib
    Threads::Threads
)
add_test(NAME test_concurrency_correctness COMMAND test_concurrency_correctness)
set_tests_properties(test_concurrency_correctness PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# 35) bench_storage_rw（简单顺序/并发写压测）
add_executable(bench_storage_rw
    unit/bench_storage_rw.cpp
)
target_link_libraries(bench_storage_rw
    storage_lib
    util_lib
    Threads::Threads
)
set_target_properties(bench_storage_rw PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/Debug
)

# 如需为 CLI/Executor 建独立目标，请在它们模块就绪后启用：
# add_executable(cli_test unit/CliTest.cpp)
# target_link_libraries(cli_test cli_lib)  # 或者链接对应核心/依赖库
# add_executable(executor_test unit/ExecutorTest.cpp)
# target_link_libraries(executor_test executor_lib)

