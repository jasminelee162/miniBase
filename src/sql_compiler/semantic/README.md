# SQL语义分析器 (Semantic Analyzer)

这是SQL编译器的语义分析器模块，负责在AST的基础上进行语义检查，并维护模式目录(Catalog)。

## 功能特点

- 表存在性检查：确保引用的表存在或不存在（对于CREATE TABLE）
- 列存在性检查：确保引用的列在表中存在
- 类型一致性检查：确保表达式和值的类型与列类型匹配
- 列数/列序检查：确保INSERT语句中的值数量与列数匹配
- 维护模式目录(Catalog)：存储表和列的元数据

## 文件结构

- `semantic_analyzer.h`/`semantic_analyzer.cpp`: 语义分析器类的声明和实现
- `semantic_test.cpp`: 测试程序
- `CMakeLists.txt`: 构建配置

## 相关模块

- `catalog/catalog.h`: 模式目录类，用于存储表和列的元数据

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
./src/frontend/semantic/semantic_test
```

## 错误处理

语义分析器会检测以下类型的错误：

1. TABLE_NOT_EXIST: 表不存在
2. TABLE_ALREADY_EXIST: 表已存在
3. COLUMN_NOT_EXIST: 列不存在
4. TYPE_MISMATCH: 类型不匹配
5. COLUMN_COUNT_MISMATCH: 列数不匹配

错误输出格式：`[错误类型, 行号, 列号] 错误描述`

## 示例

对于正确的SQL语句：
```sql
CREATE TABLE student(id INT, name VARCHAR, age INT);
INSERT INTO student(id,name,age) VALUES (1,'Alice',20);
```

输出：
```
Semantic check passed!
```

对于错误的SQL语句：
```sql
INSERT INTO student(id,name) VALUES (1,'Alice',20);
```

输出：
```
Semantic error: [COLUMN_COUNT_MISMATCH, 0, 0] Column count mismatch: expected 2, got 3
```

## 下一步

- 实现执行计划生成
- 支持更多SQL语句类型
- 增强错误诊断和恢复能力