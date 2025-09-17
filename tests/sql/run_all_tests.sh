#!/bin/bash
# ===============================================
# MiniBase 测试脚本一键执行器 (Linux/macOS)
# 自动执行所有测试脚本
# ===============================================

echo "==============================================="
echo "MiniBase 数据库系统全面测试"
echo "==============================================="

# 检查可执行文件是否存在
if [ ! -f "../../build/src/cli/Debug/minidb_cli" ] && [ ! -f "../../build/src/cli/minidb_cli" ]; then
    echo "错误：找不到 minidb_cli 可执行文件"
    echo "请确保已经编译了项目"
    exit 1
fi

# 设置可执行文件路径
if [ -f "../../build/src/cli/Debug/minidb_cli" ]; then
    CLI_PATH="../../build/src/cli/Debug/minidb_cli"
elif [ -f "../../build/src/cli/minidb_cli" ]; then
    CLI_PATH="../../build/src/cli/minidb_cli"
fi

# 创建数据目录
mkdir -p "../../data"

echo ""
echo "开始执行测试..."
echo ""

# 测试1：基础功能测试
echo "==============================================="
echo "测试1：基础功能全面测试"
echo "==============================================="
$CLI_PATH --exec --db ../../data/test_comprehensive.db < test_comprehensive_basic_with_login.sql
if [ $? -ne 0 ]; then
    echo "基础功能测试失败！"
    exit 1
fi
echo "基础功能测试完成！"

echo ""
echo "==============================================="
echo "测试2：权限系统测试"
echo "==============================================="
$CLI_PATH --exec --db ../../data/test_comprehensive.db < test_permissions_with_login.sql
if [ $? -ne 0 ]; then
    echo "权限系统测试失败！"
    exit 1
fi
echo "权限系统测试完成！"

echo ""
echo "==============================================="
echo "测试3：导入导出功能测试"
echo "==============================================="
$CLI_PATH --exec --db ../../data/test_comprehensive.db < test_import_export_with_login.sql
if [ $? -ne 0 ]; then
    echo "导入导出测试失败！"
    exit 1
fi
echo "导入导出测试完成！"

echo ""
echo "==============================================="
echo "测试4：错误处理测试"
echo "==============================================="
$CLI_PATH --exec --db ../../data/test_comprehensive.db < test_error_handling_with_login.sql
if [ $? -ne 0 ]; then
    echo "错误处理测试失败！"
    exit 1
fi
echo "错误处理测试完成！"

echo ""
echo "==============================================="
echo "所有测试完成！"
echo "==============================================="
echo "测试结果已保存到数据库文件：../../data/test_comprehensive.db"
echo "可以查看生成的导出文件验证结果"
echo ""
