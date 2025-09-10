# SQL执行计划生成器 (Plan Generator)

这个模块负责将抽象语法树(AST)转换为执行计划，是SQL编译器的最后一个阶段。执行计划是数据库引擎执行SQL语句的蓝图，它定义了如何获取、处理和操作数据。

## 功能特点

- 支持将AST转换为逻辑执行计划
- 支持CREATE TABLE、INSERT、SELECT、DELETE四类基本SQL语句
- 生成树形结构的执行计划，包含各种算子节点
- 提供计划打印功能，便于可视化和调试
- 错误处理机制，对不支持的语法或缺失的语义信息给出提示

## 文件结构

- `planner.h/cpp`: 计划生成器的核心实现，负责将AST转换为执行计划
- `plan_printer.h/cpp`: 执行计划打印器，用于可视化执行计划
- `planner_test.cpp`: 测试程序，验证执行计划生成功能

## 执行计划节点

执行计划由以下几种节点组成：

- `CreateTablePlanNode`: 创建表的计划节点
- `InsertPlanNode`: 插入数据的计划节点
- `SeqScanPlanNode`: 顺序扫描表的计划节点
- `ProjectPlanNode`: 投影操作的计划节点（选择特定列）
- `FilterPlanNode`: 过滤操作的计划节点（WHERE条件）
- `DeletePlanNode`: 删除数据的计划节点

## 构建与运行

### 构建

```bash
# 在项目根目录下执行
cmake -B build
cmake --build build
```

### 运行测试程序

```bash
# 在build目录下执行
./src/frontend/planner/planner_test
```

## 输出示例

以下是执行计划的输出示例：

```
===== Testing SQL: CREATE TABLE student(id INT, name VARCHAR, age INT); =====

Execution Plan:
CreateTable: student
  Columns: [id INT, name VARCHAR, age INT]

Plan String Representation:
CreateTable(student, [id INT, name VARCHAR, age INT])
```

```
===== Testing SQL: SELECT id, name FROM student WHERE age > 18; =====

Execution Plan:
Project: [id, name]
  SeqScan: student
    Predicate: age > 18

Plan String Representation:
Project([id, name], SeqScan(student, age > 18))
```

## 支持的SQL语法

- CREATE TABLE语句：`CREATE TABLE table_name(column_name data_type, ...);`
- INSERT语句：`INSERT INTO table_name(column_list) VALUES (value_list);`
- SELECT语句：`SELECT column_list FROM table_name WHERE condition;`
- DELETE语句：`DELETE FROM table_name WHERE condition;`

## 下一步计划

- 支持更复杂的SQL语句，如JOIN、GROUP BY、ORDER BY等
- 实现查询优化，如谓词下推、投影下推等
- 支持更多的算子，如HashJoin、Sort等
- 增强错误处理和诊断能力