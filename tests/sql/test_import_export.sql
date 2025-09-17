-- ===============================================
-- 测试脚本3：导入导出功能全面测试
-- 测试.dump和.import命令的各种场景
-- 执行方式：需要执行模式 --exec，按顺序执行各个测试区块
-- ===============================================

-- ===============================================
-- 准备工作：清理环境并登录
-- 执行前请确保使用执行模式：minidb_cli --exec --db test_import_export.db
-- ===============================================

-- 登录root用户
-- .login root root

-- 清理可能存在的测试表（注意：系统不支持DROP TABLE IF EXISTS）
-- 如果以下表存在，请手动删除：
DROP TABLE export_test_table;
DROP TABLE complex_test_table;
DROP TABLE constraint_test_table;
DROP TABLE multi_data_table;

-- ===============================================
-- 区块1：基础导出导入测试
-- ===============================================

-- 1.1 创建测试表
CREATE TABLE export_test_table (
    id INT,
    name VARCHAR(50),
    value INT,
    description VARCHAR(100)
);
-- 预期结果：表创建成功

-- 1.2 插入测试数据
INSERT INTO export_test_table (id, name, value, description) VALUES 
    (1, 'Test Item 1', 100, 'First test item'),
    (2, 'Test Item 2', 200, 'Second test item'),
    (3, 'Test Item 3', 300, 'Third test item');
-- 预期结果：插入3条记录成功

-- 1.3 验证数据
SELECT * FROM export_test_table;
-- 预期结果：显示3条记录

-- 1.4 导出到SQL文件
-- .dump tests/sql/export_basic_test.sql
-- 预期结果：导出成功，生成包含CREATE TABLE和INSERT语句的SQL文件

-- 1.5 清理当前数据
DROP TABLE export_test_table;
-- 预期结果：表删除成功

-- 1.6 验证表已删除
SHOW TABLES;
-- 预期结果：不显示export_test_table

-- 1.7 从SQL文件导入
-- .import tests/sql/export_basic_test.sql
-- 预期结果：导入成功，重新创建表和插入数据

-- 1.8 验证导入结果
SELECT * FROM export_test_table;
-- 预期结果：显示3条记录，与导出前一致

-- ===============================================
-- 区块2：复杂表结构导出导入测试
-- ===============================================

-- 2.1 创建复杂表结构
CREATE TABLE complex_test_table (
    id INT PRIMARY KEY,
    username VARCHAR(30) NOT NULL UNIQUE,
    email VARCHAR(50) UNIQUE,
    age INT DEFAULT 18,
    status VARCHAR(10) DEFAULT 'active',
    created_date VARCHAR(20),
    metadata VARCHAR(200)
);
-- 预期结果：表创建成功，包含主键、唯一约束、非空约束、默认值

-- 2.2 插入复杂数据
INSERT INTO complex_test_table (id, username, email, age, status, created_date, metadata) VALUES 
    (1, 'alice', 'alice@example.com', 25, 'active', '2024-01-01', '{"role": "admin", "permissions": ["read", "write"]}'),
    (2, 'bob', 'bob@example.com', 30, 'inactive', '2024-01-02', '{"role": "user", "permissions": ["read"]}'),
    (3, 'charlie', 'charlie@example.com', 22, 'active', '2024-01-03', '{"role": "moderator", "permissions": ["read", "moderate"]}');
-- 预期结果：插入3条记录成功

-- 2.3 验证复杂数据
SELECT * FROM complex_test_table;
-- 预期结果：显示3条记录，包含所有字段

-- 2.4 导出复杂表
-- .dump tests/sql/export_complex_test.sql
-- 预期结果：导出成功，包含完整的表结构和数据

-- 2.5 清理并重新导入
DROP TABLE complex_test_table;
-- .import tests/sql/export_complex_test.sql
-- 预期结果：导入成功，表结构和数据完全恢复

-- 2.6 验证复杂表导入结果
SELECT * FROM complex_test_table;
-- 预期结果：数据与导出前完全一致

-- ===============================================
-- 区块3：多表导出导入测试
-- ===============================================

-- 3.1 创建多个相关表
CREATE TABLE users (
    user_id INT PRIMARY KEY,
    username VARCHAR(30) NOT NULL,
    email VARCHAR(50)
);

CREATE TABLE orders (
    order_id INT PRIMARY KEY,
    user_id INT,
    product_name VARCHAR(50),
    amount INT,
    order_date VARCHAR(20)
);

CREATE TABLE products (
    product_id INT PRIMARY KEY,
    name VARCHAR(50),
    price INT,
    category VARCHAR(30)
);
-- 预期结果：3个表创建成功

-- 3.2 插入关联数据
INSERT INTO users (user_id, username, email) VALUES 
    (1, 'john', 'john@email.com'),
    (2, 'jane', 'jane@email.com'),
    (3, 'mike', 'mike@email.com');

INSERT INTO products (product_id, name, price, category) VALUES 
    (101, 'Laptop', 1000, 'Electronics'),
    (102, 'Book', 20, 'Education'),
    (103, 'Phone', 800, 'Electronics');

INSERT INTO orders (order_id, user_id, product_name, amount, order_date) VALUES 
    (1001, 1, 'Laptop', 1, '2024-01-15'),
    (1002, 2, 'Book', 3, '2024-01-16'),
    (1003, 1, 'Phone', 1, '2024-01-17'),
    (1004, 3, 'Laptop', 1, '2024-01-18');
-- 预期结果：所有数据插入成功

-- 3.3 验证多表数据
SELECT * FROM users;
SELECT * FROM products;
SELECT * FROM orders;
-- 预期结果：显示所有表的数据

-- 3.4 导出整个数据库
-- .dump tests/sql/export_multi_table_test.sql
-- 预期结果：导出成功，包含所有表的结构和数据

-- 3.5 清理所有表
DROP TABLE orders;
DROP TABLE products;
DROP TABLE users;
-- 预期结果：所有表删除成功

-- 3.6 验证清理结果
SHOW TABLES;
-- 预期结果：只显示之前创建的其他表

-- 3.7 导入多表数据
-- .import tests/sql/export_multi_table_test.sql
-- 预期结果：导入成功，所有表和数据恢复

-- 3.8 验证多表导入结果
SELECT * FROM users;
SELECT * FROM products;
SELECT * FROM orders;
-- 预期结果：所有表的数据与导出前一致

-- 3.9 测试关联查询
SELECT u.username, o.product_name, o.amount, p.price
FROM users u
JOIN orders o ON u.user_id = o.user_id
JOIN products p ON o.product_name = p.name;
-- 预期结果：显示用户、订单和产品的关联信息

-- ===============================================
-- 区块4：约束和默认值导出导入测试
-- ===============================================

-- 4.1 创建带约束的表
CREATE TABLE constraint_test_table (
    id INT PRIMARY KEY,
    name VARCHAR(30) NOT NULL UNIQUE,
    age INT DEFAULT 18,
    status VARCHAR(10) DEFAULT 'pending',
    score INT DEFAULT 0
);
-- 预期结果：表创建成功

-- 4.2 测试默认值插入
INSERT INTO constraint_test_table (id, name) VALUES (1, 'test_user_1');
-- 预期结果：插入成功，age=18, status='pending', score=0

INSERT INTO constraint_test_table (id, name, age) VALUES (2, 'test_user_2', 25);
-- 预期结果：插入成功，status='pending', score=0

INSERT INTO constraint_test_table (id, name, age, status, score) VALUES (3, 'test_user_3', 30, 'active', 95);
-- 预期结果：插入成功，所有字段都有值

-- 4.3 验证约束和默认值
SELECT * FROM constraint_test_table;
-- 预期结果：显示3条记录，默认值正确应用

-- 4.4 导出约束表
-- .dump tests/sql/export_constraint_test.sql
-- 预期结果：导出成功

-- 4.5 清理并重新导入
DROP TABLE constraint_test_table;
-- .import tests/sql/export_constraint_test.sql
-- 预期结果：导入成功

-- 4.6 验证约束表导入结果
SELECT * FROM constraint_test_table;
-- 预期结果：数据与导出前一致

-- 4.7 测试约束是否仍然有效
INSERT INTO constraint_test_table (id, name) VALUES (1, 'duplicate_name');
-- 预期结果：主键冲突错误

INSERT INTO constraint_test_table (id, name) VALUES (4, 'test_user_1');
-- 预期结果：唯一约束冲突错误

-- ===============================================
-- 区块5：大数据量导出导入测试
-- ===============================================

-- 5.1 创建大数据量表
CREATE TABLE multi_data_table (
    id INT,
    data1 VARCHAR(50),
    data2 INT,
    data3 VARCHAR(100)
);
-- 预期结果：表创建成功

-- 5.2 插入大量数据
INSERT INTO multi_data_table (id, data1, data2, data3) VALUES 
    (1, 'Data 1', 100, 'Description 1'),
    (2, 'Data 2', 200, 'Description 2'),
    (3, 'Data 3', 300, 'Description 3'),
    (4, 'Data 4', 400, 'Description 4'),
    (5, 'Data 5', 500, 'Description 5'),
    (6, 'Data 6', 600, 'Description 6'),
    (7, 'Data 7', 700, 'Description 7'),
    (8, 'Data 8', 800, 'Description 8'),
    (9, 'Data 9', 900, 'Description 9'),
    (10, 'Data 10', 1000, 'Description 10');
-- 预期结果：插入10条记录成功

-- 5.3 验证大数据量
SELECT COUNT(*) FROM multi_data_table;
-- 预期结果：显示10

-- 5.4 导出大数据量表
-- .dump tests/sql/export_large_data_test.sql
-- 预期结果：导出成功

-- 5.5 清理并重新导入
DROP TABLE multi_data_table;
-- .import tests/sql/export_large_data_test.sql
-- 预期结果：导入成功

-- 5.6 验证大数据量导入结果
SELECT COUNT(*) FROM multi_data_table;
SELECT * FROM multi_data_table WHERE id <= 5;
-- 预期结果：显示10条记录，前5条数据正确

-- ===============================================
-- 区块6：导出格式验证测试
-- ===============================================

-- 6.1 导出到不同路径测试
-- .export ./backups/export_format_test.sql
-- 预期结果：导出到指定文件成功

-- .export ./backups/
-- 预期结果：导出到指定目录成功

-- 6.2 验证导出文件内容
-- 可以手动检查导出的SQL文件，确认包含：
-- - CREATE TABLE语句
-- - INSERT INTO语句
-- - 正确的数据类型
-- - 正确的约束定义

-- ===============================================
-- 区块7：错误处理测试
-- ===============================================

-- 7.1 测试导入不存在的文件
-- .import nonexistent_file.sql
-- 预期结果：文件不存在错误

-- 7.2 测试导入格式错误的文件
-- 可以创建一个格式错误的SQL文件进行测试
-- .import tests/sql/malformed_test.sql
-- 预期结果：SQL语法错误

-- ===============================================
-- 区块8：清理和总结
-- ===============================================

-- 8.1 清理所有测试表（注意：系统不支持DROP TABLE IF EXISTS）
-- 如果以下表存在，请手动删除：
-- DROP TABLE export_test_table;
-- DROP TABLE complex_test_table;
-- DROP TABLE constraint_test_table;
-- DROP TABLE multi_data_table;
-- DROP TABLE users;
-- DROP TABLE orders;
-- DROP TABLE products;
-- 预期结果：所有测试表删除成功

-- 8.2 最终验证
SHOW TABLES;
-- 预期结果：显示清理后的表列表

-- ===============================================
-- 导入导出测试完成总结
-- 预期结果验证：
-- 1. 基础导出导入功能正常
-- 2. 复杂表结构（约束、默认值）正确导出导入
-- 3. 多表关联数据完整导出导入
-- 4. 大数据量处理正常
-- 5. 导出格式正确（CREATE TABLE + INSERT语句）
-- 6. 错误处理机制有效
-- 7. 数据完整性得到保证
-- 8. 约束和默认值在导入后仍然有效
-- ===============================================
