-- 登录 qingchen
.login
qingchen
123456

-- 创建 dba_owned_permtest 表并插入数据
CREATE TABLE dba_owned_permtest (
    id INT,
    note VARCHAR(50)
);
INSERT INTO dba_owned_permtest (id, note) VALUES (1, 'created by dba');
SELECT * FROM dba_owned_permtest;

.logout

-- 登录 root
.login
root
root

SHOW TABLES;
CREATE TABLE root_test (id INT, name VARCHAR);
.logout

-- 登录 qingchen（尝试访问 root 的表）
.login
qingchen
123456
INSERT INTO root_test VALUES (1, 'x');  -- 预期：权限拒绝
SELECT * FROM root_test;                -- 预期：可读但无数据
.users                                  -- 预期：权限拒绝
.logout

-- 登录 root，创建新用户 A
.login
root
root
.users
.create_user A 123 analyst
.back
.logout

-- 登录新用户 A
.login
A
123
SHOW TABLES;
SELECT * FROM dba_owned_permtest;       -- 预期：可以读
CREATE TABLE analyst_try (a INT, b VARCHAR(10));  -- 预期：权限拒绝
INSERT INTO dba_owned_permtest (id, note) VALUES (3, 'analyst try insert');  -- 权限拒绝
UPDATE dba_owned_permtest SET note = 'analyst try update' WHERE id = 1;      -- 权限拒绝

.exit
