// tests/simple_test_framework.h
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace SimpleTest {

class TestCase {
public:
    TestCase(const std::string& name, std::function<void()> test_func)
        : name_(name), test_func_(test_func) {}
    
    void run() {
        std::cout << "Running test: " << name_ << " ... ";
        try {
            auto start = std::chrono::high_resolution_clock::now();
            test_func_();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "PASSED (" << duration.count() << "ms)" << std::endl;
            passed_++;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << std::endl;
            failed_++;
        }
    }
    
    static int getPassed() { return passed_; }
    static int getFailed() { return failed_; }
    
private:
    std::string name_;
    std::function<void()> test_func_;
    static int passed_;
    static int failed_;
};

class TestSuite {
public:
    void addTest(const std::string& name, std::function<void()> test_func) {
        tests_.emplace_back(name, test_func);
    }
    
    void runAll() {
        std::cout << "Running " << tests_.size() << " tests..." << std::endl;
        std::cout << "==========================================" << std::endl;
        
        for (auto& test : tests_) {
            test.run();
        }
        
        std::cout << "==========================================" << std::endl;
        std::cout << "Tests passed: " << TestCase::getPassed() << std::endl;
        std::cout << "Tests failed: " << TestCase::getFailed() << std::endl;
        
        if (TestCase::getFailed() == 0) {
            std::cout << "All tests passed! ✅" << std::endl;
        } else {
            std::cout << "Some tests failed! ❌" << std::endl;
        }
    }
    
private:
    std::vector<TestCase> tests_;
};

// 简单的断言宏
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("ASSERT_TRUE failed: " #condition); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("ASSERT_FALSE failed: " #condition); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error("ASSERT_EQ failed: expected " + std::to_string(expected) + ", got " + std::to_string(actual)); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            throw std::runtime_error("ASSERT_NE failed: values are equal"); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw std::runtime_error("ASSERT_NOT_NULL failed: pointer is null"); \
        } \
    } while(0)

#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cout << "WARNING: EXPECT_TRUE failed: " #condition << std::endl; \
        } \
    } while(0)

#define EXPECT_FALSE(condition) \
    do { \
        if (condition) { \
            std::cout << "WARNING: EXPECT_FALSE failed: " #condition << std::endl; \
        } \
    } while(0)

#define EXPECT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::cout << "WARNING: EXPECT_EQ failed: expected " << (expected) << ", got " << (actual) << std::endl; \
        } \
    } while(0)

#define EXPECT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::cout << "WARNING: EXPECT_NE failed: values are equal" << std::endl; \
        } \
    } while(0)

} // namespace SimpleTest
