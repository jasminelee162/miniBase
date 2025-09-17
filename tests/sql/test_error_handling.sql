-- ===============================================
-- 测试脚本4：错误处理全面测试
-- 测试语法错误、语义错误、约束错误等各种错误情况
-- 执行方式：按顺序执行，观察错误信息和处理机制
-- ===============================================

-- ===============================================
-- 区块1：语法错误测试
-- ===============================================

-- 1.1 缺少分号错误
SELECT id FROM student
-- 预期结果：语法错误 - 缺少分号

-- 1.2 自动纠错测试
slect;
-- 预期结果：语法错误 - 拼写错误，提示正确的关键字

-- 1.3 SELECT语句错误
SELECT FROM student;
-- 预期结果：语法错误 - SELECT后缺少列名或星号

SELECT , FROM student;
-- 预期结果：语法错误 - 逗号后缺少列名

SELECT SUM( FROM student;
-- 预期结果：语法错误 - 括号不匹配

-- 1.4 FROM子句错误
SELECT id student;
-- 预期结果：语法错误 - 缺少FROM关键字

SELECT id FROM ;
-- 预期结果：语法错误 - FROM后缺少表名

-- 1.5 GROUP BY和ORDER BY错误
SELECT id FROM student GROUP id;
-- 预期结果：语法错误 - GROUP后缺少BY关键字

SELECT id FROM student ORDER id;
-- 预期结果：语法错误 - ORDER后缺少BY关键字

-- 1.6 JOIN语句错误
SELECT id FROM a JOIN b;
-- 预期结果：语法错误 - JOIN缺少ON子句

SELECT id FROM a JOIN b ON a . id = b.id;
-- 预期结果：语法错误 - 点号前后有空格

-- 1.7 括号匹配错误
CREATE TABLE t(a VARCHAR(20), b INT; -- 缺右括号
-- 预期结果：语法错误 - 缺少右括号

INSERT INTO t(a,b VALUES ('x',1)); -- 缺右括号
-- 预期结果：语法错误 - 缺少右括号

SELECT (1 + 2 FROM student; -- 缺右括号
-- 预期结果：语法错误 - 缺少右括号

-- 1.8 UPDATE语句错误
UPDATE student id = 1 WHERE name = 'A';
-- 预期结果：语法错误 - 缺少SET关键字

UPDATE student SET id WHERE name = 'A';
-- 预期结果：语法错误 - SET后缺少等号

UPDATE student SET id = WHERE name = 'A';
-- 预期结果：语法错误 - 等号后缺少值

-- ===============================================
-- 区块2：语义错误测试
-- ===============================================

-- 2.1 表不存在错误
SELECT * FROM nonexistent_table;
-- 预期结果：语义错误 - TABLE_NOT_EXIST

INSERT INTO nonexistent_table VALUES (1, 'test');
-- 预期结果：语义错误 - TABLE_NOT_EXIST

UPDATE nonexistent_table SET col = 'value';
-- 预期结果：语义错误 - TABLE_NOT_EXIST

DELETE FROM nonexistent_table WHERE id = 1;
-- 预期结果：语义错误 - TABLE_NOT_EXIST

-- 2.2 列不存在错误
CREATE TABLE test_table (id INT, name VARCHAR(50));
-- 预期结果：表创建成功

SELECT nonexistent_column FROM test_table;
-- 预期结果：语义错误 - COLUMN_NOT_EXIST

INSERT INTO test_table (nonexistent_column) VALUES ('test');
-- 预期结果：语义错误 - COLUMN_NOT_EXIST

UPDATE test_table SET nonexistent_column = 'value';
-- 预期结果：语义错误 - COLUMN_NOT_EXIST

-- 2.3 列数不匹配错误
INSERT INTO test_table (id, name) VALUES (1, 'test', 'extra');
-- 预期结果：语义错误 - COLUMN_COUNT_MISMATCH

INSERT INTO test_table (id, name) VALUES (1);
-- 预期结果：语义错误 - COLUMN_COUNT_MISMATCH

-- 2.4 表已存在错误
CREATE TABLE test_table (id INT);
-- 预期结果：语义错误 - TABLE_ALREADY_EXIST

-- ===============================================
-- 区块3：数据类型错误测试
-- ===============================================

-- 3.1 不支持的数据类型
CREATE TABLE unsupported_table (id TEXT);
-- 预期结果：语义错误 - 提示仅支持INT/VARCHAR

CREATE TABLE unsupported_table (id FLOAT);
-- 预期结果：语义错误 - 提示仅支持INT/VARCHAR

CREATE TABLE unsupported_table (id BOOLEAN);
-- 预期结果：语义错误 - 提示仅支持INT/VARCHAR

-- 3.2 类型不匹配（如果系统支持类型检查）
-- 注意：这取决于系统的类型检查严格程度
INSERT INTO test_table (id, name) VALUES ('string', 123);
-- 预期结果：可能成功（如果系统宽松）或类型不匹配错误

-- ===============================================
-- 区块4：约束错误测试
-- ===============================================

-- 4.1 创建带约束的测试表
-- 注意：如果表已存在，请先手动删除：
DROP TABLE constraint_error_table;
CREATE TABLE constraint_error_table (
    id INT PRIMARY KEY,
    username VARCHAR(30) NOT NULL UNIQUE,
    email VARCHAR(50) UNIQUE,
    age INT DEFAULT 18,
    status VARCHAR(10) DEFAULT 'active'
);
-- 预期结果：表创建成功

-- 4.2 主键约束违反
INSERT INTO constraint_error_table (id, username, email) VALUES (1, 'user1', 'user1@email.com');
-- 预期结果：插入成功

INSERT INTO constraint_error_table (id, username, email) VALUES (1, 'user2', 'user2@email.com');
-- 预期结果：约束错误 - Primary key violation on column 'id'

-- 4.3 唯一性约束违反
INSERT INTO constraint_error_table (id, username, email) VALUES (2, 'user1', 'user3@email.com');
-- 预期结果：约束错误 - Unique constraint violation on column 'username'

INSERT INTO constraint_error_table (id, username, email) VALUES (3, 'user3', 'user1@email.com');
-- 预期结果：约束错误 - Unique constraint violation on column 'email'

-- 4.4 非空约束违反
INSERT INTO constraint_error_table (id, username, email) VALUES (4, NULL, 'user4@email.com');
-- 预期结果：约束错误 - NOT NULL violation on column 'username'

-- 4.5 默认值测试（正常情况）
INSERT INTO constraint_error_table (id, username, email) VALUES (5, 'user5', 'user5@email.com');
-- 预期结果：插入成功，age=18, status='active'

-- 验证默认值
SELECT * FROM constraint_error_table WHERE id = 5;
-- 预期结果：显示记录，默认值正确应用

-- ===============================================
-- 区块5：复杂约束错误测试
-- ===============================================

-- 5.1 创建复杂约束表
-- 注意：如果表已存在，请先手动删除：DROP TABLE complex_constraint_table;
CREATE TABLE complex_constraint_table (
    id INT PRIMARY KEY,
    code VARCHAR(10) UNIQUE,
    name VARCHAR(50) NOT NULL,
    value INT DEFAULT 0,
    category VARCHAR(20) DEFAULT 'general'
);
-- 预期结果：表创建成功

-- 5.2 测试复合约束违反
INSERT INTO complex_constraint_table (id, code, name) VALUES (1, 'A001', 'Item 1');
-- 预期结果：插入成功

-- 主键重复
INSERT INTO complex_constraint_table (id, code, name) VALUES (1, 'A002', 'Item 2');
-- 预期结果：约束错误 - Primary key violation

-- 唯一码重复
INSERT INTO complex_constraint_table (id, code, name) VALUES (2, 'A001', 'Item 3');
-- 预期结果：约束错误 - Unique constraint violation on column 'code'

-- 非空违反
INSERT INTO complex_constraint_table (id, code, name) VALUES (3, 'A003', NULL);
-- 预期结果：约束错误 - NOT NULL violation on column 'name'

-- 5.3 测试默认值填充
INSERT INTO complex_constraint_table (id, code, name) VALUES (4, 'A004', 'Item 4');
-- 预期结果：插入成功，value=0, category='general'

-- 验证默认值
SELECT * FROM complex_constraint_table WHERE id = 4;
-- 预期结果：显示记录，默认值正确

-- ===============================================
-- 区块6：UPDATE和DELETE约束错误测试
-- ===============================================

-- 6.1 测试UPDATE约束违反
UPDATE constraint_error_table SET id = 1 WHERE id = 5;
-- 预期结果：约束错误 - Primary key violation（如果id=1已存在）

UPDATE constraint_error_table SET username = 'user1' WHERE id = 5;
-- 预期结果：约束错误 - Unique constraint violation on column 'username'

UPDATE constraint_error_table SET username = NULL WHERE id = 5;
-- 预期结果：约束错误 - NOT NULL violation on column 'username'

-- 6.2 测试DELETE约束（如果有外键约束）
-- 注意：当前系统可能不支持外键约束，这部分测试可能不适用

-- ===============================================
-- 区块7：权限错误测试
-- ===============================================

-- 7.1 权限相关错误（需要不同用户角色测试）
-- 这部分需要在权限测试脚本中验证，这里只做注释说明

-- 以ANALYST身份尝试创建表
-- CREATE TABLE analyst_table (id INT);
-- 预期结果：权限错误 - ANALYST不能创建表

-- 以DEVELOPER身份尝试操作其他用户的表
-- INSERT INTO other_user_table VALUES (1, 'test');
-- 预期结果：权限错误 - 不能操作其他用户的表

-- 以非DBA身份尝试用户管理
-- .users
-- 预期结果：权限错误 - 只有DBA可以管理用户

-- ===============================================
-- 区块8：系统级错误测试
-- ===============================================

-- 8.1 无效的CLI命令
-- .invalid_command
-- 预期结果：命令错误 - 未知命令

-- 8.2 文件操作错误
-- .import nonexistent_file.sql
-- 预期结果：文件错误 - 文件不存在

-- .dump /invalid/path/file.sql
-- 预期结果：文件错误 - 路径无效或权限不足

-- ===============================================
-- 区块9：错误恢复和清理测试
-- ===============================================

-- 9.1 验证错误后的系统状态
SHOW TABLES;
-- 预期结果：显示成功创建的表

-- 9.2 测试错误后的正常操作
SELECT * FROM test_table;
-- 预期结果：查询成功，显示之前插入的数据

INSERT INTO test_table (id, name) VALUES (2, 'valid data');
-- 预期结果：插入成功

-- 9.3 清理测试环境（注意：系统不支持DROP TABLE IF EXISTS）
-- 如果以下表存在，请手动删除：
DROP TABLE test_table;
DROP TABLE constraint_error_table;
DROP TABLE complex_constraint_table;
-- 预期结果：清理成功

-- 9.4 最终验证
SHOW TABLES;
-- 预期结果：显示清理后的表列表

-- ===============================================
-- 区块10：错误信息质量测试
-- ===============================================

-- 10.1 测试错误信息的准确性
-- 观察上述所有错误，验证错误信息是否：
-- - 明确指出错误类型
-- - 提供错误位置（行号、列号）
-- - 给出修复建议
-- - 使用清晰的中文描述

-- 10.2 测试错误恢复能力
-- 验证系统在遇到错误后是否能够：
-- - 继续处理后续语句
-- - 保持数据库状态一致性
-- - 不产生内存泄漏或状态混乱

-- ===============================================
-- 错误处理测试完成总结
-- 预期结果验证：
-- 1. 语法错误：提供准确的错误位置和修复建议
-- 2. 语义错误：正确识别表/列不存在、类型不匹配等问题
-- 3. 约束错误：准确报告主键、唯一性、非空约束违反
-- 4. 权限错误：正确拒绝无权限操作
-- 5. 系统错误：优雅处理文件、命令等系统级错误
-- 6. 错误恢复：错误后系统状态保持稳定
-- 7. 错误信息：清晰、准确、有帮助的错误提示
-- 8. 数据完整性：错误不会破坏现有数据
-- ===============================================
