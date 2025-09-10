# SQL语法分析器 (Parser)

这是SQL编译器的语法分析器模块，负责将Token序列解析为抽象语法树(AST)。

## 功能特点

- 支持基本SQL语句的解析：CREATE TABLE、INSERT、SELECT、DELETE
- 构建结构化的抽象语法树(AST)
- 提供错误处理和位置信息
- 支持表达式解析（比较、算术运算等）

## 文件结构

- `ast.h`/`ast.cpp`: 定义AST节点类型和结构
- `parser.h`/`parser.cpp`: 语法分析器类的声明和实现
- `ast_printer.h`/`ast_printer.cpp`: AST可视化工具
- `parser_test.cpp`: 测试程序
- `CMakeLists.txt`: 构建配置

## 如何构建

在项目根目录下执行以下命令：

```bash
mkdir build
cd build
cmake ..
make
```

## 如何运行测试

构建完成后，在build目录下执行：

```bash
./src/frontend/parser/parser_test
```

## 输出示例

对于SQL语句：`CREATE TABLE student(id INT, name VARCHAR, age INT);`

输出的AST结构如下：

```
CreateTableStatement:
  Table: student
  Columns:
    id: INT
    name: VARCHAR
    age: INT
```

## 支持的语法

1. CREATE TABLE语句：
   ```sql
   CREATE TABLE tableName(column1 TYPE, column2 TYPE, ...);
   ```

2. INSERT语句：
   ```sql
   INSERT INTO tableName(column1, column2, ...) VALUES (value1, value2, ...);
   ```

3. SELECT语句：
   ```sql
   SELECT column1, column2, ... FROM tableName WHERE condition;
   ```

4. DELETE语句：
   ```sql
   DELETE FROM tableName WHERE condition;
   ```

## 下一步

- 实现语义分析
- 构建和维护模式目录(Catalog)
- 生成执行计划