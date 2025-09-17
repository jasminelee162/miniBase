-- ===============================================
-- 测试脚本2：权限系统全面测试
-- 测试DBA、DEVELOPER、ANALYST三种角色的权限控制
-- 执行方式：按顺序执行各个角色区块，需要先创建测试用户
-- ===============================================

-- ===============================================
-- 准备工作：创建测试用户（需要DBA权限）
-- 执行前请先登录root用户：.login root root
-- ===============================================

-- 创建测试用户
-- .create_user developer_user 123456 developer
-- .create_user analyst_user 123456 analyst
-- .create_user test_dba 123456 dba

-- ===============================================
-- 区块1：DBA权限测试
-- 执行方式：.login root root 或 .login test_dba 123456
-- ===============================================

-- 1.1 DBA创建共享测试表
-- 注意：如果表已存在，请先手动删除：
DROP TABLE dba_shared_table;

CREATE TABLE dba_shared_table (
    id INT,
    data VARCHAR(50),
    owner VARCHAR(20)
);
-- 预期结果：表创建成功

-- 1.2 DBA插入测试数据
INSERT INTO dba_shared_table (id, data, owner) VALUES 
    (1, 'DBA created data', 'dba'),
    (2, 'Shared information', 'dba');
-- 预期结果：数据插入成功

-- 1.3 DBA创建自己的私有表
CREATE TABLE dba_private_table (
    id INT,
    secret_data VARCHAR(50)
);
-- 预期结果：表创建成功

INSERT INTO dba_private_table (id, secret_data) VALUES 
    (1, 'DBA private data'),
    (2, 'Confidential info');
-- 预期结果：数据插入成功

-- 1.4 DBA测试所有权限操作
-- 查询权限
SELECT * FROM dba_shared_table;
-- 预期结果：显示所有数据

-- 插入权限
INSERT INTO dba_shared_table (id, data, owner) VALUES (3, 'New DBA data', 'dba');
-- 预期结果：插入成功

-- 更新权限
UPDATE dba_shared_table SET data = 'Updated by DBA' WHERE id = 1;
-- 预期结果：更新成功

-- 删除权限
DELETE FROM dba_shared_table WHERE id = 2;
-- 预期结果：删除成功

-- 1.5 DBA用户管理权限测试
-- .users
-- 预期结果：显示用户管理界面

-- 1.6 显示当前用户信息
-- .info
-- 预期结果：显示DBA用户信息和权限

-- 1.7 显示所有表
SHOW TABLES;
-- 预期结果：显示所有表（包括其他用户创建的表）

-- 登出DBA用户
-- .logout

-- ===============================================
-- 区块2：DEVELOPER权限测试
-- 执行方式：.login developer_user 123456
-- ===============================================

-- 2.1 DEVELOPER创建自己的表
CREATE TABLE dev_own_table (
    id INT,
    dev_data VARCHAR(50)
);
-- 预期结果：表创建成功

-- 2.2 DEVELOPER操作自己的表
INSERT INTO dev_own_table (id, dev_data) VALUES 
    (1, 'Developer data 1'),
    (2, 'Developer data 2');
-- 预期结果：插入成功

UPDATE dev_own_table SET dev_data = 'Updated by developer' WHERE id = 1;
-- 预期结果：更新成功

SELECT * FROM dev_own_table;
-- 预期结果：显示开发者自己的数据

-- 2.3 DEVELOPER尝试访问DBA的表（读取权限测试）
SELECT * FROM dba_shared_table;
-- 预期结果：可能成功（取决于实现）或权限拒绝

-- 2.4 DEVELOPER尝试修改DBA的表（写权限测试）
INSERT INTO dba_shared_table (id, data, owner) VALUES (4, 'Dev try insert', 'developer');
-- 预期结果：权限拒绝 - 不能操作其他用户的表

UPDATE dba_shared_table SET data = 'Dev try update' WHERE id = 1;
-- 预期结果：权限拒绝 - 不能操作其他用户的表

DELETE FROM dba_shared_table WHERE id = 1;
-- 预期结果：权限拒绝 - 不能操作其他用户的表

-- 2.5 DEVELOPER尝试访问DBA私有表
SELECT * FROM dba_private_table;
-- 预期结果：权限拒绝 - 不能访问其他用户的私有表

-- 2.6 DEVELOPER尝试用户管理
-- .users
-- 预期结果：权限拒绝 - 只有DBA可以管理用户

-- 2.7 DEVELOPER显示表列表
SHOW TABLES;
-- 预期结果：显示所有表（但只能操作自己创建的表）

-- 2.8 DEVELOPER删除自己的表
DROP TABLE dev_own_table;
-- 预期结果：删除成功

-- 登出DEVELOPER用户
-- .logout

-- ===============================================
-- 区块3：ANALYST权限测试
-- 执行方式：.login analyst_user 123456
-- ===============================================

-- 3.1 ANALYST尝试创建表
CREATE TABLE analyst_try_table (
    id INT,
    data VARCHAR(50)
);
-- 预期结果：权限拒绝 - ANALYST只能查询，不能创建表

-- 3.2 ANALYST尝试插入数据
INSERT INTO dba_shared_table (id, data, owner) VALUES (5, 'Analyst try', 'analyst');
-- 预期结果：权限拒绝 - ANALYST不能插入数据

-- 3.3 ANALYST尝试更新数据
UPDATE dba_shared_table SET data = 'Analyst try update' WHERE id = 1;
-- 预期结果：权限拒绝 - ANALYST不能更新数据

-- 3.4 ANALYST尝试删除数据
DELETE FROM dba_shared_table WHERE id = 1;
-- 预期结果：权限拒绝 - ANALYST不能删除数据

-- 3.5 ANALYST查询权限测试
SELECT * FROM dba_shared_table;
-- 预期结果：查询成功 - ANALYST可以查看所有表的数据

SELECT id, data FROM dba_shared_table WHERE id > 0;
-- 预期结果：查询成功 - 可以执行条件查询

-- 3.6 ANALYST尝试访问DBA私有表
SELECT * FROM dba_private_table;
-- 预期结果：查询成功 - ANALYST可以查看所有表（包括私有表）

-- 3.7 ANALYST尝试用户管理
-- .users
-- 预期结果：权限拒绝 - ANALYST不能管理用户

-- 3.8 ANALYST显示表列表
SHOW TABLES;
-- 预期结果：显示所有表

-- 3.9 ANALYST尝试删除表
DROP TABLE dba_shared_table;
-- 预期结果：权限拒绝 - ANALYST不能删除表

-- 登出ANALYST用户
-- .logout

-- ===============================================
-- 区块4：权限边界测试
-- 执行方式：.login root root
-- ===============================================

-- 4.1 创建测试表用于权限边界测试
CREATE TABLE permission_test_table (
    id INT PRIMARY KEY,
    public_data VARCHAR(50),
    private_data VARCHAR(50)
);

INSERT INTO permission_test_table (id, public_data, private_data) VALUES 
    (1, 'Public info', 'Private info'),
    (2, 'Another public', 'Another private');
-- 预期结果：创建和插入成功

-- 4.2 测试跨用户表操作权限
-- 登出root，登录developer_user
-- .logout
-- .login developer_user 123456

-- 尝试操作root创建的表
SELECT * FROM permission_test_table;
-- 预期结果：可能成功（取决于实现）

INSERT INTO permission_test_table (id, public_data, private_data) VALUES (3, 'Dev insert', 'Dev private');
-- 预期结果：权限拒绝

UPDATE permission_test_table SET public_data = 'Dev update' WHERE id = 1;
-- 预期结果：权限拒绝

DELETE FROM permission_test_table WHERE id = 1;
-- 预期结果：权限拒绝

-- 4.3 测试ANALYST的只读权限
-- .logout
-- .login analyst_user 123456

SELECT * FROM permission_test_table;
-- 预期结果：查询成功

SELECT public_data FROM permission_test_table WHERE id = 1;
-- 预期结果：查询成功

-- 尝试任何写操作都应该失败
INSERT INTO permission_test_table (id, public_data) VALUES (4, 'Analyst try');
-- 预期结果：权限拒绝

-- ===============================================
-- 区块5：权限测试总结验证
-- 执行方式：.login root root
-- ===============================================

-- 5.1 验证数据完整性
SELECT * FROM dba_shared_table;
-- 预期结果：显示DBA操作后的数据状态

SELECT * FROM dba_private_table;
-- 预期结果：显示DBA私有表数据

SELECT * FROM permission_test_table;
-- 预期结果：显示权限测试表数据

-- 5.2 清理测试环境
DROP TABLE dba_shared_table;
DROP TABLE dba_private_table;
DROP TABLE permission_test_table;
-- 预期结果：清理成功

-- 5.3 显示最终表状态
SHOW TABLES;
-- 预期结果：显示清理后的表列表

-- ===============================================
-- 权限测试完成总结
-- 预期结果验证：
-- 1. DBA：拥有所有权限，可以操作所有表，可以管理用户
-- 2. DEVELOPER：可以创建和操作自己的表，不能操作其他用户的表，不能管理用户
-- 3. ANALYST：只能查询所有表，不能进行任何写操作，不能管理用户
-- 4. 权限控制正确实施，跨用户操作被正确拒绝
-- 5. 数据完整性得到保护
-- ===============================================
