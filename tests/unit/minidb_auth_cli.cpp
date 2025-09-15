#include "../../src/cli/AuthCLI.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static void print_help() {
    std::cout << "MiniDB 认证版 CLI\n"
              << "Usage: minidb_auth_cli\n"
              << "Features:\n"
              << "  - 用户认证登录\n"
              << "  - 基于角色的权限控制\n"
              << "  - 数据库操作\n"
              << "  - 用户管理\n"
              << "\n"
              << "Default admin: root / root\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 检查命令行参数
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
    }

    try {
        // 创建并运行认证CLI
        minidb::AuthCLI cli;
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "未知错误发生" << std::endl;
        return 1;
    }

    return 0;
}
