CREATE PROCEDURE test_proc (teacher_name VARCHAR)
BEGIN
    INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES (1, teacher_name, 'Math', 5);
END;
