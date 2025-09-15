-- 缺少分号
SELECT id FROM student
INSERT INTO student VALUES (1,'A',20)
--自动纠错
slect;
-- SELECT 缺列名或星号
SELECT FROM student;
SELECT , FROM student;
SELECT SUM( FROM student;
-- 缺少 FROM
SELECT id student;
-- FROM 缺表名
SELECT id FROM ;
-- GROUP/ORDER 缺 BY
SELECT id FROM student GROUP id;
SELECT id FROM student ORDER id;
-- JOIN 缺 ON 或条件不完整
SELECT id FROM a JOIN b;
SELECT id FROM a JOIN b ON a . id = b.id;
-- 括号缺失
CREATE TABLE t(a VARCHAR(20), b INT; -- 缺右括号
INSERT INTO t(a,b VALUES ('x',1)); -- 缺右括号
SELECT (1 + 2 FROM student; --缺右括号
-- 数据类型错误或不支持
CREATE TABLE t(a TEXT); 
-- // 期望提示仅支持 INT/VARCHAR
-- UPDATE 语句缺 SET/等号/值
UPDATE student id = 1 WHERE name = 'A';
UPDATE student SET id WHERE name = 'A';
UPDATE student SET id = WHERE name = 'A';

-- 触发的典型提示示例
-- 语句末尾多余内容：会提示“检查是否多写了额外内容，或在上一条语句末尾缺少 ';'”
-- 缺少 BY：会提示在 GROUP/ORDER 之后补上 BY
-- JOIN 缺 ON：提示“JOIN 缺少 ON 子句”，并给出在 JOIN 条件前补 ON
-- 缺少右括号/左括号：提示在相应位置补上 ')' 或 '('
-- SELECT 缺列：提示“可使用 * 或列名，如: id, name”
-- 列定义缺类型：提示“目前支持 INT 或 VARCHAR[(长度)]”