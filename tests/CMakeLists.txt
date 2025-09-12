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


# 如需为 CLI/Executor 建独立目标，请在它们模块就绪后启用：
# add_executable(cli_test unit/CliTest.cpp)
# target_link_libraries(cli_test cli_lib)  # 或者链接对应核心/依赖库
# add_executable(executor_test unit/ExecutorTest.cpp)
# target_link_libraries(executor_test executor_lib)

