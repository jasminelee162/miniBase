CREATE TABLE teachers (teacher_id INT, full_name VARCHAR(0), subject VARCHAR(0), experience INT);

INSERT INTO teachers VALUES();
INSERT INTO teachers VALUES();
INSERT INTO teachers VALUES();
INSERT INTO teachers VALUES();
INSERT INTO teachers VALUES('202', 'Bob Johnson', 'English', '8');

CREATE TABLE __users__ (username VARCHAR(64), password_hash VARCHAR(128), role INT, created_at BIGINT, last_login BIGINT);

INSERT INTO __users__ VALUES('qingchen', '17790324078706895114', '1', '1758025794', '1758029609');
INSERT INTO __users__ VALUES('gq', '618478622264491733', '2', '1758033392', '0');
INSERT INTO __users__ VALUES('root', '11769310039851165637', '0', '1758025733', '1758035660');

CREATE TABLE teachers_916 (teacher_id INT, full_name VARCHAR(0), subject VARCHAR(0), experience INT);


CREATE TABLE dba_owned_permtest (id INT, note VARCHAR(0));

INSERT INTO dba_owned_permtest VALUES('1', 'created by dba');

CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(-1) UNIQUE NOT NULL, age INT DEFAULT '18');

INSERT INTO users VALUES('1', 'Alice', '18');
INSERT INTO users VALUES('2', 'Bob', '20');
INSERT INTO users VALUES('4', 'Carol', '18');

CREATE TABLE root_test (id INT, name VARCHAR(0));


CREATE TABLE import_test (teacher_id INT, full_name VARCHAR(100);

INSERT INTO import_test VALUES();
INSERT INTO import_test VALUES();
INSERT INTO import_test VALUES('201', 'Alice Smith');

CREATE TABLE prod (code INT UNIQUE, title VARCHAR(-1) NOT NULL, price INT DEFAULT '100');

INSERT INTO prod VALUES('10', 'P1', '100');
INSERT INTO prod VALUES('11', 'P3', '100');

