#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include "role_manager.h"

// 前向声明
namespace minidb
{
    class StorageEngine;
    class Catalog;
}

namespace minidb
{

    struct UserRecord
    {
        std::string username;
        std::string password_hash;
        Role role;
        time_t created_at;
        time_t last_login;

        UserRecord() : role(Role::ANALYST), created_at(0), last_login(0) {}

        UserRecord(const std::string &uname, const std::string &phash, Role r)
            : username(uname), password_hash(phash), role(r), created_at(time(nullptr)), last_login(0) {}
    };

    class UserStorageManager
    {
    private:
        StorageEngine *storage_engine_;
        Catalog *catalog_;
        bool initialized_;

        // 用户表相关常量
        static const std::string USER_TABLE_NAME;
        static const std::string USERNAME_COL;
        static const std::string PASSWORD_COL;
        static const std::string ROLE_COL;
        static const std::string CREATED_AT_COL;
        static const std::string LAST_LOGIN_COL;

        // 内部方法
        bool createUserTable();
        bool insertUserRecord(const UserRecord &user);
        bool updateUserRecord(const UserRecord &user);
        bool deleteUserRecord(const std::string &username);
        std::vector<UserRecord> getAllUserRecords();
        UserRecord getUserRecord(const std::string &username);

        // 序列化/反序列化
        std::string serializeUserRecord(const UserRecord &user) const;
        UserRecord deserializeUserRecord(const std::string &data) const;

    public:
        UserStorageManager(StorageEngine *storage_engine, Catalog *catalog);

        // 初始化
        bool initialize();

        // 用户管理
        bool createUser(const std::string &username, const std::string &password, Role role);
        bool userExists(const std::string &username);
        bool authenticate(const std::string &username, const std::string &password);
        bool deleteUser(const std::string &username);
        bool updateUserPassword(const std::string &username, const std::string &new_password);
        bool updateUserRole(const std::string &username, Role new_role);

        // 用户查询
        std::vector<std::string> listUsers();
        std::vector<UserRecord> getAllUsers();
        UserRecord getUserInfo(const std::string &username);
        Role getUserRole(const std::string &username);

        // 工具方法
        std::string hashPassword(const std::string &password);
        bool isInitialized() const { return initialized_; }
    };

} // namespace minidb
