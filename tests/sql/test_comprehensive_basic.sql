-- ===============================================
-- 测试脚本1：基础功能全面测试（使用root账号）
-- 测试所有支持的SQL语句类型：基础CRUD、高级查询、存储过程、约束支持
-- 执行方式：.login root root 然后执行此脚本
-- ===============================================
 .\build\src\cli\Debug\minidb_cli.exe --exec --db data/test_comprehensive.db
 root
 root
-- 如果表存在，请手动删除：
DROP TABLE students;
DROP TABLE teachers;
DROP TABLE departments;
DROP TABLE employees;
DROP TABLE users;
DROP TABLE products;
DROP TABLE test_proc;

-- ===============================================
-- 1. 基础CRUD操作测试
-- ===============================================

-- 1.1 CREATE TABLE - 基础表创建
CREATE TABLE students (
    student_id INT,
    name VARCHAR(50),
    age INT,
    major VARCHAR(30)
);
-- 预期结果：表创建成功

-- 1.2 INSERT - 单条插入
INSERT INTO students (student_id, name, age, major) VALUES (1001, 'Alice Johnson', 20, 'Computer Science');
-- 预期结果：插入成功

-- 1.3 INSERT - 多条插入
INSERT INTO students (student_id, name, age, major) VALUES 
    (1002, 'Bob Smith', 21, 'Mathematics'),
    (1003, 'Carol Davis', 19, 'Physics'),
    (1004, 'David Wilson', 22, 'Computer Science');
-- 预期结果：插入3条记录成功

-- 1.4 SELECT - 基础查询
SELECT * FROM students;
-- 预期结果：显示所有4条学生记录

-- 1.5 SELECT - 条件查询
SELECT name, major FROM students WHERE age > 20;
-- 预期结果：显示年龄大于20的学生姓名和专业

-- 1.6 UPDATE - 更新数据
UPDATE students SET age = 23 WHERE name = 'David Wilson';
-- 预期结果：David Wilson的年龄更新为23

-- 1.7 DELETE - 删除数据
DELETE FROM students WHERE student_id = 1003;
-- 预期结果：删除Carol Davis的记录

-- 验证更新和删除结果
SELECT * FROM students;
-- 预期结果：显示3条记录，David年龄为23，Carol记录被删除

-- ===============================================
-- 2. 高级查询功能测试
-- ===============================================

-- 2.1 创建测试数据表
CREATE TABLE teachers (
    teacher_id INT,
    name VARCHAR(50),
    subject VARCHAR(30),
    experience INT,
    salary INT
);

INSERT INTO teachers (teacher_id, name, subject, experience, salary) VALUES
    (2001, 'Prof. Smith', 'Mathematics', 10, 80000),
    (2002, 'Prof. Johnson', 'Computer Science', 15, 95000),
    (2003, 'Prof. Brown', 'Physics', 8, 75000),
    (2004, 'Prof. Davis', 'Mathematics', 12, 85000),
    (2005, 'Prof. Wilson', 'Computer Science', 6, 70000);

-- 2.2 GROUP BY - 分组查询(x)
SELECT subject, COUNT(*) as teacher_count, AVG(experience) as avg_experience 
FROM teachers 
GROUP BY subject;
-- 预期结果：按学科分组，显示每科的教师数量和平均经验

-- 2.3 HAVING - 分组过滤（x）
SELECT subject, COUNT(*) as teacher_count, AVG(salary) as avg_salary 
FROM teachers 
GROUP BY subject 
HAVING COUNT(*) > 1;
-- 预期结果：只显示教师数量大于1的学科

-- 2.4 ORDER BY - 排序查询
SELECT name, subject, experience 
FROM teachers 
ORDER BY experience DESC;
-- 预期结果：按经验降序排列教师

-- 2.5 复合查询 - GROUP BY + ORDER BY (x)
SELECT subject, SUM(salary) as total_salary 
FROM teachers 
GROUP BY subject 
ORDER BY total_salary DESC;
-- 预期结果：按学科分组计算总薪资，并按总薪资降序排列

-- ===============================================
-- 3. JOIN操作测试
-- ===============================================

-- 3.1 创建关联表
CREATE TABLE departments (
    dept_id INT,
    dept_name VARCHAR(30),
    building VARCHAR(20),
    budget INT
);

INSERT INTO departments (dept_id, dept_name, building, budget) VALUES
    (1, 'Computer Science', 'Tech Building', 500000),
    (2, 'Mathematics', 'Science Hall', 300000),
    (3, 'Physics', 'Research Center', 400000);

-- 3.2 INNER JOIN (x)
SELECT t.name, t.subject, d.building, d.budget
FROM teachers t
JOIN departments d ON t.subject = d.dept_name;
-- 预期结果：显示教师及其对应部门的建筑和预算信息

-- ===============================================
-- 4. 存储过程测试
-- ===============================================

-- 4.1 创建存储过程
CREATE PROCEDURE add_teacher (teacher_name VARCHAR, teacher_subject VARCHAR, teacher_exp INT)
BEGIN
    INSERT INTO teachers (teacher_id, name, subject, experience, salary) 
    VALUES (2000 + (SELECT COUNT(*) FROM teachers) + 1, teacher_name, teacher_subject, teacher_exp, 60000);
END;
-- 预期结果：存储过程创建成功

-- 4.2 调用存储过程
CALL add_teacher('Prof. New', 'Chemistry', 5);
-- 预期结果：添加新教师记录

-- 验证存储过程执行结果
SELECT * FROM teachers WHERE name = 'Prof. New';
-- 预期结果：显示新添加的教师记录

-- ===============================================
-- 5. 约束功能测试
-- ===============================================

-- 5.1 创建带约束的表
CREATE TABLE users (
    user_id INT PRIMARY KEY,
    username VARCHAR(30) NOT NULL UNIQUE,
    email VARCHAR(50) UNIQUE,
    age INT DEFAULT 18,
    status VARCHAR(10) DEFAULT 'active'
);
-- 预期结果：表创建成功，包含主键、非空、唯一、默认值约束

-- 5.2 测试默认值约束
INSERT INTO users (user_id, username) VALUES (1, 'alice');
-- 预期结果：插入成功，age默认为18，status默认为'active'

-- 5.3 测试主键约束（正常情况）
INSERT INTO users (user_id, username, email, age) VALUES (2, 'bob', 'bob@email.com', 25);
-- 预期结果：插入成功

-- 5.4 测试主键约束（违反情况）
INSERT INTO users (user_id, username) VALUES (1, 'charlie');
-- 预期结果：报错 - Primary key violation

-- 5.5 测试唯一性约束（正常情况）
INSERT INTO users (user_id, username, email) VALUES (3, 'david', 'david@email.com');
-- 预期结果：插入成功

-- 5.6 测试唯一性约束（违反情况）
INSERT INTO users (user_id, username, email) VALUES (4, 'alice', 'eve@email.com');
-- 预期结果：报错 - Unique constraint violation on username

-- 5.7 测试非空约束（违反情况）
INSERT INTO users (user_id, username) VALUES (5, NULL);
-- 预期结果：报错 - NOT NULL violation on username

-- 5.8 测试默认值填充
INSERT INTO users (user_id, username, email) VALUES (6, 'frank', 'frank@email.com');
-- 预期结果：插入成功，age和status使用默认值

-- 验证约束测试结果
SELECT * FROM users;
-- 预期结果：显示成功插入的用户记录

-- ===============================================
-- 6. 复杂查询组合测试
-- ===============================================

-- 6.1 创建员工表进行复杂测试
CREATE TABLE employees (
    emp_id INT,
    name VARCHAR(50),
    department VARCHAR(30),
    position VARCHAR(30),
    salary INT,
    hire_year INT
);

INSERT INTO employees (emp_id, name, department, position, salary, hire_year) VALUES
    (3001, 'John Doe', 'Engineering', 'Senior Developer', 90000, 2020),
    (3002, 'Jane Smith', 'Engineering', 'Junior Developer', 65000, 2022),
    (3003, 'Mike Johnson', 'Marketing', 'Manager', 80000, 2019),
    (3004, 'Sarah Wilson', 'Marketing', 'Analyst', 55000, 2021),
    (3005, 'Tom Brown', 'Engineering', 'Architect', 110000, 2018),
    (3006, 'Lisa Davis', 'HR', 'Manager', 75000, 2020),
    (3007, 'Chris Lee', 'Engineering', 'Developer', 70000, 2021);

-- 6.2 复杂聚合查询
SELECT 
    department,
    COUNT(*) as employee_count,
    AVG(salary) as avg_salary,
    MAX(salary) as max_salary,
    MIN(salary) as min_salary
FROM employees 
WHERE hire_year >= 2020
GROUP BY department
HAVING COUNT(*) >= 2
ORDER BY avg_salary DESC;
-- 预期结果：显示2020年后入职、部门人数>=2的部门统计信息，按平均薪资降序

-- 6.3 子查询和条件组合
SELECT name, department, salary
FROM employees
WHERE salary > (SELECT AVG(salary) FROM employees)
ORDER BY salary DESC;
-- 预期结果：显示薪资高于平均薪资的员工，按薪资降序

-- ===============================================
-- 7. 系统命令测试
-- ===============================================

-- 7.1 显示所有表
SHOW TABLES;
-- 预期结果：显示所有创建的表

-- 7.2 清理测试环境（可选）
-- 如果需要清理，请手动执行：
DROP TABLE students;
DROP TABLE teachers;
DROP TABLE departments;
DROP TABLE users;
DROP TABLE employees;

-- ===============================================
-- 测试完成总结
-- 预期结果：所有基础功能测试通过，包括：
-- 1. 基础CRUD操作
-- 2. 高级查询（GROUP BY, HAVING, ORDER BY）
-- 3. JOIN操作
-- 4. 存储过程创建和调用
-- 5. 约束功能（PRIMARY KEY, UNIQUE, NOT NULL, DEFAULT）
-- 6. 复杂查询组合
-- 7. 系统命令
-- ===============================================
