# 简单的测试配置，不使用GoogleTest
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

