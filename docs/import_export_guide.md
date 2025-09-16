# MiniDB 导入导出功能使用说明

## 功能概述

MiniDB 现在支持数据库的导入和导出功能，允许用户将数据库结构和数据保存为SQL文件，或者从SQL文件恢复数据库。

## 使用方法

### 1. 启动CLI

```bash
# 使用执行模式启动CLI（导入导出功能需要执行模式）
./minidb_cli --exec --db your_database.db
```

### 2. 导出数据库

在CLI中使用 `.dump` 命令导出数据库：

```
>> .dump output_file.sql
```

这将把当前数据库的所有表结构和数据导出到 `output_file.sql` 文件中。

### 3. 导入数据库

在CLI中使用 `.import` 命令导入SQL文件：

```
>> .import input_file.sql
```

这将执行SQL文件中的所有CREATE TABLE和INSERT语句，重建数据库。

## 支持的SQL语句

### 导入功能支持：
- `CREATE TABLE` - 创建表结构
- `INSERT INTO` - 插入数据

### 导出功能生成：
- `CREATE TABLE` - 表结构定义
- `INSERT INTO` - 数据插入语句

## 示例

### 1. 创建测试数据

首先创建一个SQL文件 `test_data.sql`：

```sql
CREATE TABLE students (
    id INT,
    name VARCHAR(50),
    age INT
);

INSERT INTO students VALUES('1', 'Alice', '20');
INSERT INTO students VALUES('2', 'Bob', '22');
INSERT INTO students VALUES('3', 'Charlie', '21');
```

### 2. 导入数据

```bash
./minidb_cli --exec --db mydb.db
>> .import test_data.sql
Database imported from: test_data.sql
```

### 3. 导出数据

```
>> .dump backup.sql
Database exported to: backup.sql
```

### 4. 验证导出文件

导出的 `backup.sql` 文件内容：

```sql
CREATE TABLE students (id INT, name VARCHAR(50), age INT);

INSERT INTO students VALUES('1', 'Alice', '20');
INSERT INTO students VALUES('2', 'Bob', '22');
INSERT INTO students VALUES('3', 'Charlie', '21');
```

## 注意事项

1. **执行模式要求**：导入导出功能需要在执行模式下运行（使用 `--exec` 参数）
2. **文件路径**：确保指定的文件路径存在且可写
3. **SQL语法**：导入的SQL文件必须符合MiniDB支持的语法格式
4. **数据类型**：目前支持 INT、VARCHAR、CHAR、DOUBLE 等基本数据类型

## 错误处理

- 如果文件不存在或无法访问，会显示相应的错误信息
- 如果SQL语法不正确，会跳过有问题的语句并继续处理
- 导入过程中如果表已存在，可能会产生冲突

## 技术实现

- **SQLDumper**: 负责将数据库导出为SQL格式
- **SQLImporter**: 负责解析SQL文件并执行相应的数据库操作
- **集成到CLI**: 通过命令行接口提供用户友好的操作方式

## 扩展功能

未来可以考虑添加：
- 支持更多SQL语句类型（UPDATE、DELETE等）
- 增量导入导出
- 压缩格式支持
- 进度显示
- 错误恢复机制
