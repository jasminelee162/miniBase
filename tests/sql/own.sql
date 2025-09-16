-- =============================
-- 权限验证测试（请按角色分别执行）
-- 使用CLI先登录对应用户，再执行相应区块：
--   1) .login  <DBA用户>       → 执行【DBA区块】创建 dba_owned_permtest（不要清理）
--   2) .login  <DEVELOPER用户> → 执行【DEVELOPER区块】对 dba_owned_permtest 进行 R/W 测试（应被拒绝）
--   3) .login  <ANALYST用户>   → 执行【ANALYST区块】仅允许 SELECT，其他应被拒绝
-- 说明：解析器支持 CREATE/SELECT/INSERT/UPDATE/DELETE/SHOW/DROP/PROCEDURE/CALL 等；无 CREATE USER/GRANT/REVOKE SQL，用户管理请用 CLI 命令。
-- =============================

-- =============================
-- 【DBA区块】（DBA 先执行本区块，保留 dba_owned_permtest 供后续测试）
-- =============================
DROP TABLE IF EXISTS dba_owned_permtest;
CREATE TABLE dba_owned_permtest (
    id INT,
    note VARCHAR(50)
);
INSERT INTO dba_owned_permtest (id, note) VALUES (1, 'created by dba');
SELECT * FROM dba_owned_permtest;

-- DBA 自己的典型全权限操作（应全部允许）
CREATE TABLE dba_tmp (
    k INT,
    v VARCHAR(20)
);
INSERT INTO dba_tmp (k, v) VALUES (1, 'x'), (2, 'y');
UPDATE dba_tmp SET v = 'z' WHERE k = 2;
DELETE FROM dba_tmp WHERE k = 1;
SELECT * FROM dba_tmp;
SHOW TABLES;
DROP TABLE dba_tmp;

-- （可选）测试完成后由 DBA 清理：
-- DROP TABLE dba_owned_permtest;

-- =============================
-- 【DEVELOPER区块】（开发者）
-- 预期：只能操作自己创建的表；对 DBA 拥有的 dba_owned_permtest 的 INSERT/UPDATE/DELETE 应被拒绝，SELECT 取决于实现（若按表级限制则也拒绝）
-- =============================
-- 开发者创建并操作自己的表（应允许）
DROP TABLE IF EXISTS dev_own;
CREATE TABLE dev_own (
    id INT,
    msg VARCHAR(50)
);
INSERT INTO dev_own (id, msg) VALUES (10, 'hello'), (20, 'world');
UPDATE dev_own SET msg = 'world!' WHERE id = 20;
DELETE FROM dev_own WHERE id = 10;
SELECT * FROM dev_own;

-- 访问 DBA 拥有的表：预期为权限拒绝（至少对 INSERT/UPDATE/DELETE）
-- 若系统允许开发者对他人表 SELECT，则下条可能成功；若实施了严格表属主限制，则会拒绝
SELECT * FROM dba_owned_permtest;
INSERT INTO dba_owned_permtest (id, note) VALUES (2, 'dev try insert');
UPDATE dba_owned_permtest SET note = 'dev try update' WHERE id = 1;
DELETE FROM dba_owned_permtest WHERE id = 1;

SHOW TABLES;

-- =============================
-- 【ANALYST区块】（分析师）
-- 预期：仅允许 SELECT；CREATE/INSERT/UPDATE/DELETE 应被拒绝
-- =============================
-- 尝试只读访问 DBA 表（应允许 SELECT）
SELECT * FROM dba_owned_permtest;

-- 下列写操作与建表应被拒绝
CREATE TABLE analyst_try (
    a INT,
    b VARCHAR(10)
);
INSERT INTO dba_owned_permtest (id, note) VALUES (3, 'analyst try insert');
UPDATE dba_owned_permtest SET note = 'analyst try update' WHERE id = 1;
DELETE FROM dba_owned_permtest WHERE id = 1;

SHOW TABLES;