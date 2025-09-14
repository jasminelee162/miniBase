
CREATE TABLE employees_914 (emp_id INT,emp_name VARCHAR(50), age INT);
CREATE TABLE departments_914 (dept_id INT,dept_name VARCHAR(50));

INSERT INTO employees_914 (emp_id, emp_name, age) VALUES (19, 'qcq', 25);
INSERT INTO employees_914 (emp_id, emp_name, age) VALUES (2, 'Bob', 30);

INSERT INTO departments_914 (dept_id, dept_name) VALUES (101, 'Engineering');
INSERT INTO departments_914 (dept_id, dept_name) VALUES (102, 'Marketing');

SELECT * FROM employees_914; 
SELECT * FROM departments_914;
SELECT * FROM employees_914 WHERE age > 26;
SELECT * FROM employees_914 WHERE age = 26;
SELECT * FROM employees_914 WHERE age < 26;
SELECT emp_id, emp_name FROM employees_914;
SELECT dept_id, dept_name FROM departments_914;
SELECT dept_id, dept_name FROM departments_914 WHERE dept_name = 'Marketing';

UPDATE employees_914 SET age = 26 WHERE emp_name = 'qcq';
UPDATE departments_914 SET dept_name = 'HR' WHERE dept_id = 101;

SELECT * FROM employees_914;
SELECT * FROM departments_914;

DELETE FROM employees_914 WHERE emp_id = 2;
DELETE FROM departments_914 WHERE dept_id = 102;
