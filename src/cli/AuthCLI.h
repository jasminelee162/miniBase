#pragma once
#include <string>
#include <memory>
#include "../auth/auth_service.h"
#include "../storage/storage_engine.h"
#include "../catalog/catalog.h"

namespace minidb {

class AuthCLI {
private:
    std::unique_ptr<StorageEngine> storage_engine_;
    std::unique_ptr<Catalog> catalog_;
    std::unique_ptr<AuthService> auth_service_;
    bool is_authenticated_;
    std::string current_user_;

public:
    AuthCLI();
    ~AuthCLI() = default;

    // 认证相关
    bool login(const std::string& username, const std::string& password);
    void logout();
    bool isAuthenticated() const;
    std::string getCurrentUser() const;
    std::string getCurrentUserRole() const;

    // CLI交互
    void run();
    void showLoginPrompt();
    void showMainMenu();
    void processCommand(const std::string& command);

    // 权限检查
    bool checkPermission(Permission permission) const;
    void showPermissionDenied(const std::string& operation) const;

private:
    void initializeDatabase();
    void showWelcomeMessage();
    void showHelp();
    void showUserInfo();
    void showTables();
    void createTable(const std::string& table_name);
    void insertData(const std::string& table_name);
    void selectData(const std::string& table_name);
    void deleteData(const std::string& table_name);
    void manageUsers();
};

} // namespace minidb
