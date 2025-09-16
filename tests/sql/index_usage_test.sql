DROP TABLE IF EXISTS teachers_idx2;
CREATE TABLE teachers_idx2 (
    id INT,
    name VARCHAR(64),
    age INT
);

-- 先创建索引
CREATE INDEX idx_teachers2_age ON teachers_idx2(age) USING BPLUS;

-- 再插入数据（INSERT 会自动维护索引）
INSERT INTO teachers_idx2 (id, name, age) VALUES
    (1, 'Alice', 30),
    (2, 'Bob', 28),
    (3, 'Carol', 30),
    (4, 'Dave', 35),
    (5, 'Erin', 42),
    (6, 'Frank', 30);

-- 测试查询（应命中索引）
SELECT id, name, age FROM teachers_idx2 WHERE age = 30;
SELECT id, name, age FROM teachers_idx2 ORDER BY age;
SELECT * FROM teachers_idx2;