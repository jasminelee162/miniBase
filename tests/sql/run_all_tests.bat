@echo off
REM ===============================================
REM MiniBase 测试脚本一键执行器
REM 自动执行所有测试脚本
REM ===============================================

echo ===============================================
echo MiniBase 数据库系统全面测试
echo ===============================================

REM 检查可执行文件是否存在
if not exist "..\..\build\src\cli\Debug\minidb_cli.exe" (
    echo 错误：找不到 minidb_cli.exe
    echo 请确保已经编译了项目
    pause
    exit /b 1
)

REM 创建数据目录
if not exist "..\..\data" mkdir "..\..\data"

echo.
echo 开始执行测试...
echo.

REM 测试1：基础功能测试
echo ===============================================
echo 测试1：基础功能全面测试
echo ===============================================
..\..\build\src\cli\Debug\minidb_cli.exe --exec --db ..\..\data\test_comprehensive.db < test_comprehensive_basic_with_login.sql
if %errorlevel% neq 0 (
    echo 基础功能测试失败！
    pause
    exit /b 1
)
echo 基础功能测试完成！

echo.
echo ===============================================
echo 测试2：权限系统测试
echo ===============================================
..\..\build\src\cli\Debug\minidb_cli.exe --exec --db ..\..\data\test_comprehensive.db < test_permissions_with_login.sql
if %errorlevel% neq 0 (
    echo 权限系统测试失败！
    pause
    exit /b 1
)
echo 权限系统测试完成！

echo.
echo ===============================================
echo 测试3：导入导出功能测试
echo ===============================================
..\..\build\src\cli\Debug\minidb_cli.exe --exec --db ..\..\data\test_comprehensive.db < test_import_export_with_login.sql
if %errorlevel% neq 0 (
    echo 导入导出测试失败！
    pause
    exit /b 1
)
echo 导入导出测试完成！

echo.
echo ===============================================
echo 测试4：错误处理测试
echo ===============================================
..\..\build\src\cli\Debug\minidb_cli.exe --exec --db ..\..\data\test_comprehensive.db < test_error_handling_with_login.sql
if %errorlevel% neq 0 (
    echo 错误处理测试失败！
    pause
    exit /b 1
)
echo 错误处理测试完成！

echo.
echo ===============================================
echo 所有测试完成！
echo ===============================================
echo 测试结果已保存到数据库文件：..\..\data\test_comprehensive.db
echo 可以查看生成的导出文件验证结果
echo.

pause
