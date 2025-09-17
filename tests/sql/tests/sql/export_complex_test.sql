CREATE TABLE export_test_table (id INT, name VARCHAR(-1);


CREATE TABLE __users__ (username VARCHAR(64), password_hash VARCHAR(128), role INT, created_at BIGINT, last_login BIGINT);

INSERT INTO __users__ VALUES('root', '11769310039851165637', '0', '1758079288', '1758080163');
INSERT INTO __users__ VALUES('root', '11769310039851165637', '0', '1758079288', '1758080163');

CREATE TABLE employees (emp_id INT, name VARCHAR(-1), department VARCHAR(-1), position VARCHAR(-1), salary INT, hire_year INT);

INSERT INTO employees VALUES('3001', 'John Doe', 'Engineering', 'Senior Developer', '90000', '2020');
INSERT INTO employees VALUES('3002', 'Jane Smith', 'Engineering', 'Junior Developer', '65000', '2022');
INSERT INTO employees VALUES('3003', 'Mike Johnson', 'Marketing', 'Manager', '80000', '2019');
INSERT INTO employees VALUES('3004', 'Sarah Wilson', 'Marketing', 'Analyst', '55000', '2021');
INSERT INTO employees VALUES('3005', 'Tom Brown', 'Engineering', 'Architect', '110000', '2018');
INSERT INTO employees VALUES('3006', 'Lisa Davis', 'HR', 'Manager', '75000', '2020');
INSERT INTO employees VALUES('3007', 'Chris Lee', 'Engineering', 'Developer', '70000', '2021');
INSERT INTO employees VALUES('3001', 'John Doe', 'Engineering', 'Senior Developer', '90000', '2020');
INSERT INTO employees VALUES('3002', 'Jane Smith', 'Engineering', 'Junior Developer', '65000', '2022');
INSERT INTO employees VALUES('3003', 'Mike Johnson', 'Marketing', 'Manager', '80000', '2019');
INSERT INTO employees VALUES('3004', 'Sarah Wilson', 'Marketing', 'Analyst', '55000', '2021');
INSERT INTO employees VALUES('3005', 'Tom Brown', 'Engineering', 'Architect', '110000', '2018');
INSERT INTO employees VALUES('3006', 'Lisa Davis', 'HR', 'Manager', '75000', '2020');
INSERT INTO employees VALUES('3007', 'Chris Lee', 'Engineering', 'Developer', '70000', '2021');

CREATE TABLE dba_shared_table (id INT, data VARCHAR(-1), owner VARCHAR(-1));

INSERT INTO dba_shared_table VALUES('1', 'Updated by DBA', 'dba');
INSERT INTO dba_shared_table VALUES('3', 'New DBA data', 'dba');
INSERT INTO dba_shared_table VALUES('1', 'Updated by DBA', 'dba');
INSERT INTO dba_shared_table VALUES('3', 'New DBA data', 'dba');

CREATE TABLE dba_private_table (id INT, secret_data VARCHAR(-1));

INSERT INTO dba_private_table VALUES('1', 'Updated by DBA');
INSERT INTO dba_private_table VALUES('1', 'Updated by DBA');

CREATE TABLE complex_test_table (id INT PRIMARY KEY, username VARCHAR(-1) UNIQUE NOT NULL, email VARCHAR(-1) UNIQUE, age INT DEFAULT '18', status VARCHAR(-1) DEFAULT 'active', created_date VARCHAR(-1), metadata VARCHAR(-1));

INSERT INTO complex_test_table VALUES('1', 'alice', 'alice@example.com', '25', 'active', '2024-01-01', '{"role": "admin", "permissions": ["read", "write"]}');
INSERT INTO complex_test_table VALUES('2', 'bob', 'bob@example.com', '30', 'inactive', '2024-01-02', '{"role": "user", "permissions": ["read"]}');
INSERT INTO complex_test_table VALUES('3', 'charlie', 'charlie@example.com', '22', 'active', '2024-01-03', '{"role": "moderator", "permissions": ["read", "moderate"]}');

