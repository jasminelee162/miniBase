-- Constraint feature tests

-- Clean up if exists
DROP TABLE users;

-- Create with PRIMARY KEY, UNIQUE, NOT NULL, DEFAULT
CREATE TABLE users(
  id INT PRIMARY KEY,
  name VARCHAR(50) NOT NULL UNIQUE,
  age INT DEFAULT 18
);

-- 1) DEFAULT works when NULL provided
INSERT INTO users VALUES(1,'Alice',NULL);

SELECT * FROM users; --结果age默认是18

-- 2) PRIMARY KEY violation (duplicate id)
-- expect error
-- [ERROR] Unique constraint violation on column 'id'
INSERT INTO users VALUES(1,'Bob',20);

-- 3) NOT NULL violation (name cannot be NULL)
-- expect error
-- [ERROR] NOT NULL violation on column 'name'
INSERT INTO users VALUES(2,NULL,20);

-- 4) Valid insert
INSERT INTO users VALUES(2,'Bob',20);

SELECT * FROM users; --结果新增一条数据

-- 5) UNIQUE violation on name
-- expect error
-- [ERROR] Unique constraint violation on column 'name'
INSERT INTO users VALUES(3,'Bob',25);

-- 6) DEFAULT fills when age omitted (column list)
INSERT INTO users (id, name) VALUES(4,'Carol');

-- 7) Show results
SELECT * FROM users;

-- Additional table to test UNIQUE and DEFAULT on non-PK
DROP TABLE prod;
CREATE TABLE prod(
  code INT UNIQUE,
  title VARCHAR(20) NOT NULL,
  price INT DEFAULT 100
);

-- 8) DEFAULT for price
INSERT INTO prod VALUES(10,'P1',NULL);

 select * from prod; --结果price默认是100

-- 9) UNIQUE violation on code
-- expect error
-- [ERROR] Unique constraint violation on column 'code'
INSERT INTO prod VALUES(10,'P2',200);

-- 10) DEFAULT when omitting price
INSERT INTO prod (code,title) VALUES(11,'P3');

-- 11) Show results
SELECT * FROM prod;


