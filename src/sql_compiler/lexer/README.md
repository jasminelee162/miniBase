# SQL词法分析器 (Lexer)

这是SQL编译器的词法分析器模块，负责将SQL语句分解为Token序列。

## 功能特点

- 识别SQL关键字（SELECT, FROM, WHERE, CREATE, TABLE等）
- 识别标识符（表名、列名）
- 识别常量（整数、字符串）
- 识别运算符（=, <, >, <=, >=, !=, +, -, *, /）
- 识别分隔符（;, ,, (, ), .）
- 提供错误处理和位置信息

## 文件结构

- `token.h`: 定义Token类型和结构
- `lexer.h`: 词法分析器类的声明
- `lexer.cpp`: 词法分析器类的实现
- `lexer_test.cpp`: 测试程序
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
./src/frontend/lexer/lexer_test
```

## 输出格式

每个Token以四元组形式输出：`[种别码, 词素值, 行号, 列号]`

例如：
```
[KEYWORD_CREATE, CREATE, 1, 1]
[KEYWORD_TABLE, TABLE, 1, 8]
[IDENTIFIER, student, 1, 14]
...
```

## 下一步

- 实现语法分析器 (Parser)
- 构建抽象语法树 (AST)
- 实现语义分析
- 生成执行计划