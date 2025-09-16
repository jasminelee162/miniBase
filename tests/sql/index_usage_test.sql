-- 索引使用测试脚本（B+ 树）

-- 1) 准备表
DROP TABLE IF EXISTS teachers_idx;
CREATE TABLE teachers_idx (
    id INT,
    name VARCHAR(64),
    age INT
);

-- 2) 插入一些测试数据
INSERT INTO teachers_idx (id, name, age) VALUES
    (1, 'Alice', 30),
    (2, 'Bob', 28),
    (3, 'Carol', 30),
    (4, 'Dave', 35),
    (5, 'Erin', 42),
    (6, 'Frank', 30);

-- 3) 创建单列 B+ 树索引（在 age 列上）
CREATE INDEX idx_teachers_age ON teachers_idx(age) USING BPLUS;

-- 4) 利用索引的等值查询（应命中索引）
SELECT id, name, age FROM teachers_idx WHERE age = 30;

-- 5) 排序（如果实现支持按索引顺序读取，可减少排序开销）
SELECT id, name, age FROM teachers_idx ORDER BY age;

-- 6) 删除命中索引的行，验证删除流程与索引维护
DELETE FROM teachers_idx WHERE age = 30;

-- 7) 再查询确认删除结果
SELECT id, name, age FROM teachers_idx ORDER BY id;


