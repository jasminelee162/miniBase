DROP TABLE IF EXISTS teachers;
DROP TABLE students;

SHOW TABLES;

CREATE TABLE teachers_916 (
    teacher_id INT,
    full_name VARCHAR(100),
    subject VARCHAR(50),
    experience INT
);
-- 七条记录
INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES (201, 'Alice Smith', 'Math', 5);
INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES (202, 'Bob Johnson', 'English', 8);
INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES (203, 'Carol Lee', 'Physics', 3);
INSERT INTO teachers (teacher_id, full_name, subject, experience) 
VALUES 
    (204, 'David Kim', 'History', 10),
    (205, 'Eva Brown', 'Chemistry', 6),
    (206, 'Frank Green', 'Math', 12),
    (207, 'Grace White', 'English', 15);
-- 增加三条记录
/* Comment: Adding three more teachers to the table */
INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES
    (208, 'Helen Park', 'Physics', 7),   
    (209, 'Ian Scott', 'History', 9),    
    (210, 'Jack Brown', 'Chemistry', 11); 

SELECT * FROM teachers; 

SELECT full_name, subject FROM teachers;

SELECT full_name, subject FROM teachers WHERE experience > 5; 


UPDATE teachers SET experience = 9 WHERE full_name = 'Bob Johnson';


DELETE FROM teachers WHERE teacher_id = 203;

SELECT * FROM teachers;

SELECT subject, SUM(experience) AS total_exp FROM teachers GROUP BY subject;

SELECT subject, SUM(experience) AS total_exp FROM teachers GROUP BY subject HAVING total_exp > 15;

SELECT * FROM teachers ORDER BY experience ASC;
SELECT subject, SUM(experience) AS total FROM teachers GROUP BY subject ORDER BY total DESC;

CREATE TABLE departments (
    dept_id INT,
    dept_name VARCHAR(50),
    building VARCHAR(50)
);

INSERT INTO departments (dept_id, dept_name, building) VALUES
    (1, 'Math', 'Science Hall'),
    (2, 'English', 'Humanities Hall'),
    (3, 'Physics', 'Research Center'),
    (4, 'History', 'Liberal Arts Hall'),
    (5, 'Chemistry', 'Science Hall');
SELECT * FROM departments;
SELECT * FROM teachers JOIN departments ON teachers.subject = departments.dept_name;

-- 存储过程
CREATE PROCEDURE test_proc (teacher_name VARCHAR)
BEGIN
    INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES (1, teacher_name, 'Math', 5);
END;

CALL test_proc('John Smith'); 
--------------------------------

DROP TABLE IF EXISTS employees_914;

CREATE TABLE employees_914 (
    emp_id INT,
    emp_name VARCHAR(50),
    department VARCHAR(50),
    age INT
);

INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (301, 'John Doe', 'Engineering', 28);
INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (302, 'Jane Roe', 'Marketing', 35);
INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (303, 'Sam Black', 'HR', 40);
INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (304, 'Lily White', 'Engineering', 25);
INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (305, 'Tom Brown', 'Marketing', 30);
INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (306, 'Emma Green', 'HR', 45);
INSERT INTO employees_914 (emp_id, emp_name, department, age) VALUES (307, 'Alex Gray', 'Engineering', 32);

SELECT emp_name, department FROM employees_914;

SELECT emp_name, age FROM employees_914 WHERE age > 30;

UPDATE employees_914 SET age = 29 WHERE emp_name = 'Lily White';

DELETE FROM employees_914 WHERE emp_id = 303;

SELECT emp_name, age FROM employees_914;

SELECT department, SUM(age) AS total_age FROM employees_914 GROUP BY department;

SELECT department, SUM(age) AS total_age FROM employees_914 GROUP BY department HAVING total_age > 60;

