#!/bin/bash
# 测试导入导出功能的脚本

echo "=== MiniDB 导入导出功能测试 ==="

# 创建测试数据库
TEST_DB="test_import_export.db"

echo "1. 清理旧的测试数据库..."
rm -f "$TEST_DB"

echo "2. 启动MiniDB CLI进行测试..."
echo "测试命令序列："
echo "  - 导入测试SQL文件"
echo "  - 导出数据库"
echo "  - 退出"

# 创建命令输入文件
cat > test_commands.txt << EOF
.import test_import.sql
.dump test_export.sql
.exit
EOF

echo "3. 执行测试..."
# 注意：这里需要根据实际的CLI可执行文件路径调整
# ./build/src/cli/minidb_cli --exec --db "$TEST_DB" < test_commands.txt

echo "4. 检查导出文件..."
if [ -f "test_export.sql" ]; then
    echo "✓ 导出文件创建成功"
    echo "导出文件内容预览："
    head -20 test_export.sql
else
    echo "✗ 导出文件创建失败"
fi

echo "5. 清理测试文件..."
rm -f test_commands.txt

echo "=== 测试完成 ==="
