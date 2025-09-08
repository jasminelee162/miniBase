# Fetch GoogleTest with consistent settings
include(FetchContent)

# Ensure MSVC uses the same runtime library settings as our build
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB UNIT_TESTS unit/*.cpp)

add_executable(storage_test ${UNIT_TESTS})

# Use gtest_main provided by fetchcontent
find_package(Threads REQUIRED)

target_link_libraries(storage_test
    storage_lib
    util_lib
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

add_test(NAME storage_test COMMAND storage_test)

