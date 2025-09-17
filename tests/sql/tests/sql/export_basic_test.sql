CREATE TABLE export_test_table (id INT, name VARCHAR(-1), value INT, description VARCHAR(-1));

INSERT INTO export_test_table VALUES('1', 'Test Item 1', '100', 'First test item');
INSERT INTO export_test_table VALUES('2', 'Test Item 2', '200', 'Second test item');
INSERT INTO export_test_table VALUES('3', 'Test Item 3', '300', 'Third test item');

CREATE TABLE __users__ (username VARCHAR(64), password_hash VARCHAR(128), role INT, created_at BIGINT, last_login BIGINT);

INSERT INTO __users__ VALUES('root', '11769310039851165637', '0', '1758079288', '1758080163');

CREATE TABLE employees (emp_id INT, name VARCHAR(-1), department VARCHAR(-1), position VARCHAR(-1), salary INT, hire_year INT);

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

CREATE TABLE dba_private_table (id INT, secret_data VARCHAR(-1));

INSERT INTO dba_private_table VALUES('1', 'Updated by DBA');

