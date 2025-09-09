#include <iostream>
#include "../../src/engine/operators/row.h"
int main()
{
    std::cout << "Running Row Test..." << std::endl;

    // 1. 构造 Row
    Row r = {{"id", "1"}, {"name", "Alice"}, {"age", "20"}};

    // 2. 打印 Row
    std::cout << "Row toString: " << r.toString() << std::endl;

    // 3. 获取字段值
    std::cout << "id = " << r.getValue("id") << std::endl;
    std::cout << "name = " << r.getValue("name") << std::endl;
    std::cout << "age = " << r.getValue("age") << std::endl;

    // 4. 访问不存在的字段
    std::cout << "non_exist = " << r.getValue("non_exist") << std::endl;

    std::cout << "Row Test finished." << std::endl;
    return 0;
}
