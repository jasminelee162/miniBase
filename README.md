# miniBase

miniBase 是一个面向教学与实训的迷你关系型数据库系统，旨在帮助学习者贯通编译原理、操作系统与数据库三门核心课程。从 SQL 语言的编译到底层存储与执行引擎，覆盖编译器、存储、执行引擎与 CLI 的端到端实现。

## 项目结构

```
miniBase/
  ├─ src/
  │  ├─ util/                 # 公共配置、通用类型
  │  ├─ storage/              # 操作系统模块（页式存储、缓冲池、磁盘管理）
  │  │  ├─ page/              # Page、DiskManager
  │  │  └─ buffer/            # BufferPoolManager、LRUReplacer
  │  ├─ catalog/              # 系统目录 (表/列元数据)
  │  ├─ sql_compiler/         # SQL 编译器模块（lexer/parser/semantic/planner）
  │  ├─ engine/               # 数据库执行引擎
  │  └─ main.cpp              # 最小可运行示例
  ├─ tests/                   # 单元测试
  │  └─ unit/                 # 存储模块测试等
  ├─ data/                    # 运行时生成的数据文件
  ├─ docs/                    # 文档与图示
  └─ CMakeLists.txt           # 顶层构建脚本
```

## 功能模块

### 1. SQL 编译器模块（`src/sql_compiler/`）

负责将 SQL 文本转换为逻辑执行计划，为后续执行引擎与存储引擎提供输入。

- **词法分析（Lexer）**：识别关键字、标识符、常量、运算符、分隔符，输出 Token 流（[TokenType, Lexeme, 行, 列]）。
- **语法分析（Parser）**：基于 SQL 子集文法（支持 CREATE/INSERT/SELECT/DELETE），构建 AST，支持错误定位与期望符号提示。
- **语义分析（Semantic）**：表/列存在性、类型一致性、列数/列序检查；维护 Catalog；输出语义结果或错误信息。
- **计划生成（Planner）**：将 AST 转换为逻辑算子树（如 CreateTable、Insert、SeqScan、Filter、Project），输出为树结构/JSON/S-表达式之一。
- **测试**：提供单元测试（lexer_test、parser_test、semantic_test、planner_test），保证各阶段正确性。

### 2. 操作系统模块（存储子系统，`src/storage/`）

提供页式存储、缓冲池与磁盘管理，是数据库系统的高性能持久化基础。

- **Page**：固定大小（默认 4KB）页数据结构，含脏页标记与 pin 计数、读写锁等。
- **DiskManager**：页级同步 I/O、页分配/统计、Flush/Shutdown。
- **LRUReplacer**：线程安全 LRU，O(1) 插入/命中/淘汰。
- **BufferPoolManager**：页面缓存、替换、刷盘与统计（命中率）。
- **StorageEngine**：对外统一接口（Get/Create/Put/Remove/Checkpoint/统计）。
- **测试**：支持 ctest 单元测试，覆盖核心存储流程。

运行示例（`src/main.cpp`）：
```
创建一页 → 写入 "Hello MiniBase" → 读取校验 → 打印缓冲池大小与命中率。
预期输出：
Hello MiniBase
BufferPoolSize=64, HitRate=1
```
运行目录将生成 `minidb.data` 数据文件。

### 3. 数据库模块（`src/catalog/`、`src/engine/`、`src/cli/`）

在编译器与存储之上，构建数据库的高层能力，包括系统目录、执行引擎与交互界面。

- **Catalog（系统目录）**：维护表、列元数据（表名、列名、类型），持久化为特殊元数据表。
- **执行引擎（Engine）**：实现火山模型基本算子（SeqScan/Filter/Project、CreateTable/Insert），串联存储引擎。
- **CLI**：交互式 Shell，接收 SQL → 编译 → 执行 → 返回结果或错误信息。
- **持久性**：表与元数据经页式存储持久化，程序重启后依然可查询。

## 导入导出功能

通过 CLI 支持 `.dump` 和 `.import` 命令，支持 SQL 文件的导入导出，并集成错误提示及帮助信息。详见项目 docs 目录中的文档。

## 项目文档 (docs/)

下列文档位于仓库的 docs/ 目录，已作为超链接添加，方便直接在 README 中访问：

- [bin_file_layout.md](./docs/bin_file_layout.md) - 二进制文件/页布局说明
- [cli权限使用.md](./docs/cli权限使用.md) - CLI 使用与权限说明
- [import_export_guide.md](./docs/import_export_guide.md) - 导入导出使用指南
- [storage_api_content.md](./docs/storage_api_content.md) - 存储 API 与 .bin 三页划分说明
- [使用文档.md](./docs/使用文档.md) - 常规使用与连接数据库示例

## 构建与运行

### 依赖

- CMake（3.16+）
- C++17 编译器（Windows: MSVC 2022；Linux/macOS: Clang/GCC）

### 构建（以 Windows/MSVC 为例）

```bash
cmake -DBUILD_TESTS=ON -S . -B build
cmake --build build --config Debug
```

### 运行最小示例

```bash
./build/src/Debug/minidb.exe
# 预期输出：
# Hello MiniBase
# BufferPoolSize=64, HitRate=1
```

### 运行单元测试

```bash
cd build
cmake --build . --config Debug --target storage_test
tests\Debug\storage_test.exe
# 运行 sql_compiler 模块的测试
cmake --build . --config Debug --target lexer_test
tests\Debug\lexer_test.exe
...
ctest -C Debug --output-on-failure
```

## 实训映射与验收要点

- **SQL 编译器**：Token 流→AST→语义检查→逻辑计划；错误应含类型/位置/原因。
- **操作系统模块**：页分配/读写、缓冲池与 LRU、命中率统计与替换/写回日志。
- **数据库模块**：Catalog 元数据、火山模型算子、持久化与重启后可查询。


## 亮点与特性：

- **完整的元数据管理**：支持 Meta Superblock 的全生命周期管理，元数据持久化增强。
- **目录页支持**：为 Catalog 模块迁移到页式存储打好基础。
- **B+树索引增强**：支持增删改查（CRUD）、模板化多键类型（不仅仅是 int32，还可自定义类型）、范围查询、键统计等，代码复用性强。
- **页内数据操作便利工具**：如 AppendRecordToPage、GetPageRecords、InitializeDataPage，极大简化页操作流程，减少重复代码。
- **向后兼容**：新功能与原有代码完全兼容，便于在实际教学/开发场景平滑升级。
- **系统健壮性与可靠性**：详细文档和 checklist，易于集成和二次开发。
- **WebUI/AI 接口扩展**：有 WebUI 示例和 AI 对接接口，为可视化和智能化扩展预留空间。

## 版权与许可

本项目仅用于教学与学习目的。

## Benchmark script (Windows PowerShell)

A helper script `bench.ps1` automates generating big data, importing, indexing, and benchmarking.

Example:

```powershell
# From repo root in PowerShell
./bench.ps1 -Rows 1000000 -Runs 5 -UserId 12345 `
./bench.ps1 -Rows 100000 -Runs 5 -UserId 12345 `
  -Db "E:\Code\Project_grade3\SX\new\miniBase\data\mini.db" `
  -Cli "E:\Code\Project_grade3\SX\new\miniBase\build\src\cli\Debug\minidb_cli.exe" `
  -OutDir "E:\Code\Project_grade3\SX\new\miniBase\bench_out"
```

Outputs a CSV with per-run timing under `bench_out/` and an `import.log` with import output. Adjust parameters to test different scales and selectivities.
