#include <iostream>
#include <set>

enum class TestEnum {
    A, B, C
};

// 为TestEnum添加比较操作符
inline bool operator<(TestEnum a, TestEnum b) {
    return static_cast<int>(a) < static_cast<int>(b);
}

int main() {
    std::cout << "=== 测试简单set ===" << std::endl;
    
    // 1. 创建set
    std::set<TestEnum> test_set = {TestEnum::A, TestEnum::B, TestEnum::C};
    
    // 2. 测试count方法
    std::cout << "set中A的数量: " << test_set.count(TestEnum::A) << std::endl;
    std::cout << "set中B的数量: " << test_set.count(TestEnum::B) << std::endl;
    std::cout << "set中C的数量: " << test_set.count(TestEnum::C) << std::endl;
    std::cout << "set中D的数量: " << test_set.count(static_cast<TestEnum>(3)) << std::endl;
    
    // 3. 测试find方法
    std::cout << "set中A的find结果: " << (test_set.find(TestEnum::A) != test_set.end() ? "找到" : "未找到") << std::endl;
    std::cout << "set中B的find结果: " << (test_set.find(TestEnum::B) != test_set.end() ? "找到" : "未找到") << std::endl;
    std::cout << "set中C的find结果: " << (test_set.find(TestEnum::C) != test_set.end() ? "找到" : "未找到") << std::endl;
    
    // 4. 测试枚举值
    std::cout << "TestEnum::A值: " << static_cast<int>(TestEnum::A) << std::endl;
    std::cout << "TestEnum::B值: " << static_cast<int>(TestEnum::B) << std::endl;
    std::cout << "TestEnum::C值: " << static_cast<int>(TestEnum::C) << std::endl;
    
    // 5. 测试比较操作符
    std::cout << "A < B: " << (TestEnum::A < TestEnum::B) << std::endl;
    std::cout << "B < C: " << (TestEnum::B < TestEnum::C) << std::endl;
    std::cout << "A < C: " << (TestEnum::A < TestEnum::C) << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}
