#include "../../src/auth/role_manager.h"
#include <iostream>
#include <set>

using namespace minidb;

int main() {
    std::cout << "=== 测试Permission比较操作符 ===" << std::endl;
    
    // 1. 测试Permission枚举值
    std::cout << "\n1. 测试Permission枚举值" << std::endl;
    std::cout << "CREATE_TABLE值: " << static_cast<int>(Permission::CREATE_TABLE) << std::endl;
    std::cout << "DROP_TABLE值: " << static_cast<int>(Permission::DROP_TABLE) << std::endl;
    std::cout << "ALTER_TABLE值: " << static_cast<int>(Permission::ALTER_TABLE) << std::endl;
    std::cout << "CREATE_INDEX值: " << static_cast<int>(Permission::CREATE_INDEX) << std::endl;
    std::cout << "DROP_INDEX值: " << static_cast<int>(Permission::DROP_INDEX) << std::endl;
    std::cout << "SELECT值: " << static_cast<int>(Permission::SELECT) << std::endl;
    std::cout << "INSERT值: " << static_cast<int>(Permission::INSERT) << std::endl;
    std::cout << "UPDATE值: " << static_cast<int>(Permission::UPDATE) << std::endl;
    std::cout << "DELETE值: " << static_cast<int>(Permission::DELETE) << std::endl;
    std::cout << "CREATE_USER值: " << static_cast<int>(Permission::CREATE_USER) << std::endl;
    std::cout << "DROP_USER值: " << static_cast<int>(Permission::DROP_USER) << std::endl;
    std::cout << "GRANT值: " << static_cast<int>(Permission::GRANT) << std::endl;
    std::cout << "REVOKE值: " << static_cast<int>(Permission::REVOKE) << std::endl;
    std::cout << "SHOW_PROCESSES值: " << static_cast<int>(Permission::SHOW_PROCESSES) << std::endl;
    std::cout << "KILL_PROCESS值: " << static_cast<int>(Permission::KILL_PROCESS) << std::endl;
    std::cout << "SHOW_VARIABLES值: " << static_cast<int>(Permission::SHOW_VARIABLES) << std::endl;
    std::cout << "SET_VARIABLES值: " << static_cast<int>(Permission::SET_VARIABLES) << std::endl;
    
    // 2. 测试Permission比较操作符
    std::cout << "\n2. 测试Permission比较操作符" << std::endl;
    std::cout << "CREATE_TABLE < DROP_TABLE: " << (Permission::CREATE_TABLE < Permission::DROP_TABLE) << std::endl;
    std::cout << "SELECT < INSERT: " << (Permission::SELECT < Permission::INSERT) << std::endl;
    std::cout << "INSERT < UPDATE: " << (Permission::INSERT < Permission::UPDATE) << std::endl;
    std::cout << "CREATE_USER < DROP_USER: " << (Permission::CREATE_USER < Permission::DROP_USER) << std::endl;
    
    // 3. 测试set的count方法
    std::cout << "\n3. 测试set的count方法" << std::endl;
    std::set<Permission> test_set = {Permission::INSERT, Permission::SELECT, Permission::CREATE_USER};
    std::cout << "set中INSERT数量: " << test_set.count(Permission::INSERT) << std::endl;
    std::cout << "set中SELECT数量: " << test_set.count(Permission::SELECT) << std::endl;
    std::cout << "set中CREATE_USER数量: " << test_set.count(Permission::CREATE_USER) << std::endl;
    std::cout << "set中UPDATE数量: " << test_set.count(Permission::UPDATE) << std::endl;
    
    // 4. 测试set的find方法
    std::cout << "\n4. 测试set的find方法" << std::endl;
    std::cout << "set中INSERT find结果: " << (test_set.find(Permission::INSERT) != test_set.end() ? "找到" : "未找到") << std::endl;
    std::cout << "set中SELECT find结果: " << (test_set.find(Permission::SELECT) != test_set.end() ? "找到" : "未找到") << std::endl;
    std::cout << "set中CREATE_USER find结果: " << (test_set.find(Permission::CREATE_USER) != test_set.end() ? "找到" : "未找到") << std::endl;
    std::cout << "set中UPDATE find结果: " << (test_set.find(Permission::UPDATE) != test_set.end() ? "找到" : "未找到") << std::endl;
    
    // 5. 测试set的size
    std::cout << "\n5. 测试set的size" << std::endl;
    std::cout << "set大小: " << test_set.size() << std::endl;
    
    // 6. 遍历set内容
    std::cout << "\n6. 遍历set内容" << std::endl;
    for (const auto& perm : test_set) {
        std::cout << "权限: " << static_cast<int>(perm) << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}
