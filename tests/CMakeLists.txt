# 简单的测试配置，不使用GoogleTest
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

# 如需为 CLI/Executor 建独立目标，请在它们模块就绪后启用：
# add_executable(cli_test unit/CliTest.cpp)
# target_link_libraries(cli_test cli_lib)  # 或者链接对应核心/依赖库
# add_executable(executor_test unit/ExecutorTest.cpp)
# target_link_libraries(executor_test executor_lib)

