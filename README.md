# miniBase

一个面向教学与实训的迷你关系型数据库系统，分三大模块循序渐进构建：SQL 编译器模块、操作系统（存储）模块、数据库模块。目标是贯通编译原理、操作系统与数据库三门核心课程，从语言到系统的完整实现路径。

## 目录结构
```
miniBase/
  ├─ src/
  │  ├─ util/                 # 公共配置、通用类型
  │  ├─ storage/              # 操作系统模块（页式存储、缓冲池、磁盘管理）
  │  │  ├─ page/              # Page、DiskManager
  │  │  └─ buffer/            # BufferPoolManager、LRUReplacer
  │  ├─ catalog/              # 系统目录 (表/列元数据)
  │  ├─ frontend/             # SQL 编译器模块（待实现）
  │  ├─ engine/               # 数据库执行引擎
  │  └─ main.cpp              # 最小可运行示例
  ├─ tests/                   # 单元测试
  │  └─ unit/                 # 存储模块测试等
  ├─ data/                    # 数据文件目录（运行时生成）
  ├─ docs/                    # 文档与图示（可选）
  └─ CMakeLists.txt           # 顶层构建脚本
```

## 模块一：SQL 编译器模块（Frontend）
该模块将 SQL 文本转换为逻辑执行计划（Plan），为后续执行引擎与存储引擎提供输入。

- 目标能力：
  - 词法分析：识别关键字、标识符、常量、运算符、分隔符，输出 Token 流，[TokenType, Lexeme, 行, 列]。
  - 语法分析：基于 SQL 子集文法（CREATE/INSERT/SELECT/DELETE）构建 AST，支持错误定位与期望符号提示。
  - 语义分析：表/列存在性、类型一致性、列数/列序检查；维护 Catalog；输出语义结果或错误信息。
  - 计划生成：将 AST 转换为逻辑算子树（CreateTable、Insert、SeqScan、Filter、Project），输出为树/JSON/S-表达式之一。

- 当前进度：
  - 目录与构建脚手架已就绪（`src/frontend/`）。
  - 具体实现待补充（Lexer/Parser/Semantic/PlanBuilder）。

- 建议路线：
  1) 定义 SQL 子集与 Token/AST/错误模型；
  2) 实现 Lexer → Parser（递归下降或 LL(1)）；
  3) 语义分析接入 Catalog；
  4) 生成计划（SeqScan/Filter/Project 等）。

## 模块二：操作系统模块（存储子系统）
该模块提供页式存储、缓冲池与磁盘管理，是数据库系统的高性能持久化基础。
- 生成构建目录，先配置再编译：
`cd E:\SepExp\miniBase`
`cmake -S . -B .\build -DCMAKE_BUILD_TYPE=Debug`
`cd .\build`
`cmake --build . --config Debug -j`
- 进行存储模块的ctest单元测试：
`ctest -C Debug -R storage_test -V`

- 已实现组件：
  - Page（`src/storage/page/page.h`）：固定大小（默认 4KB）页数据结构，含脏页标记与 pin 计数、读写锁等。
  - DiskManager（`src/storage/page/disk_manager.*`）：页级同步 I/O、页分配/统计、Flush/Shutdown。
  - LRUReplacer（`src/storage/buffer/lru_replacer.*`）：线程安全 LRU，O(1) 插入/命中/淘汰。
  - BufferPoolManager（`src/storage/buffer/buffer_pool_manager.*`）：页面缓存、替换、刷盘与统计（命中率）。
  - StorageEngine（`src/storage/storage_engine.*`）：对外统一接口（Get/Create/Put/Remove/Checkpoint/统计）。

- 对外接口一览（路径 / 函数签名 / 说明）
  - 磁盘层（真实文件 I/O）`src/storage/page/disk_manager.h`
    - `Status ReadPage(page_id_t page_id, char* page_data)`：按页号读取 4KB 到缓冲区；越界自动零填充。
    - `Status WritePage(page_id_t page_id, const char* page_data)`：将 4KB 缓冲区写回对应页。
    - `page_id_t AllocatePage()` / `void DeallocatePage(page_id_t)`：分配与回收页号（回收进入空闲队列，可复用）。
  - 缓存层（缓冲池）`src/storage/buffer/buffer_pool_manager.h`
    - `Page* FetchPage(page_id_t page_id)`：从缓存取页；未命中则从磁盘装入并可能触发替换。
    - `Page* NewPage(page_id_t* out_id)`：分配新页并装入缓冲池，返回可写 `Page*`。
    - `bool UnpinPage(page_id_t page_id, bool is_dirty)`：页用完解 pin；`is_dirty=true` 标记脏页。
    - `bool FlushPage(page_id_t page_id)` / `void FlushAllPages()`：刷单页 / 全部脏页到磁盘。
    - `void SetPolicy(ReplacementPolicy)`：切换替换策略（`LRU` 或 `FIFO`）。
    - 统计：`double GetHitRate()`、`size_t GetNumReplacements()`、`size_t GetNumWritebacks()`。
  - 统一门面（推荐上层仅依赖此层）`src/storage/storage_engine.h`
    - `Page* GetPage(page_id_t)`、`Page* CreatePage(page_id_t*)`、`bool PutPage(page_id_t,bool is_dirty)`、`bool RemovePage(page_id_t)`。
    - `void Checkpoint()` / `void Shutdown()`：全量刷脏 / 关闭。
    - `double GetCacheHitRate()`、`size_t GetBufferPoolSize()`、`size_t GetNumReplacements()`、`size_t GetNumWritebacks()`。
    - `void SetReplacementPolicy(ReplacementPolicy)`：设置 LRU/FIFO。

- 使用示例（典型调用流程）
  1) 获取或创建页 → 写入数据 → 标记脏并解 pin（PutPage）
  2) 需要时刷新（Checkpoint/FlushPage）
  3) 读取同一页验证内容；查看统计与策略切换
- 创建引擎
  `StorageEngine engine("minidb.data", 128);`  128是缓存池大小，可修改
- 修改替换策略
  `engine.SetReplacementPolicy(ReplacementPolicy::FIFO);`
  或者
  `engine.SetReplacementPolicy(ReplacementPolicy::LRU);`默认

- 可选日志（便于实验与报告）
  - 在 `src/util/config.h` 将 `ENABLE_STORAGE_LOG` 设为 `true` 后重编译，可输出 `[DM]` 读/写、`[BPM]` 写回日志到标准错误流，便于观察淘汰与 I/O 行为。

- 单元测试：
  - `tests/unit/storage_test.cpp` 覆盖基本页操作、并发访问、缓存命中率、LRU 策略。
  - 使用 CMake FetchContent 引入 googletest，自动匹配运行库配置；Windows 下默认使用 MSVC `/utf-8`。

- 运行示例（`src/main.cpp`）：
  - 创建一页 → 写入 "Hello MiniBase" → 读取校验 → 打印缓冲池大小与命中率。
  - 正常输出示例：
    - 第一行：`Hello MiniBase`
    - 第二行：`BufferPoolSize=64, HitRate=1`
    - 运行目录将生成 `minidb.data` 数据文件。

## 模块三：数据库模块（Catalog/Engine/CLI）
在编译器与存储之上，构建数据库上层能力：系统目录、执行引擎与交互界面。

- 目标能力：
  - Catalog（系统目录）：维护表/列元数据（表名、列名、类型），持久化为一张特殊表。
  - 执行引擎（Engine）：实现火山模型基本算子（SeqScan/Filter/Project、CreateTable/Insert）。
  - CLI：交互式 Shell，接收 SQL → 编译 → 执行 → 返回结果或错误信息。
  - 数据持久性：表与元数据经页式存储持久化，程序重启不丢失。

- 当前进度：
  - 目录与构建脚手架已就绪：`src/catalog/`、`src/engine/`、`src/cli/`。
  - 具体实现待完成与联调。

## 构建与运行

### 依赖
- CMake（3.16+）
- C++17 编译器（Windows: MSVC 2022；Linux/macOS: Clang/GCC）

### 构建（Windows/MSVC 示例）
```bash
cmake -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

### 运行最小示例
```bash
E:\SepExp\miniBase\build\src\Debug\minidb.exe
# 预期输出：
# Hello MiniBase
# BufferPoolSize=64, HitRate=1
```

### 运行单元测试
```bash
cd build
cmake --build . --config Debug --target storage_test
tests\\Debug\\storage_test.exe
# 或
ctest -C Debug --output-on-failure
```

## 实训映射与验收要点
- SQL 编译器：Token 流→AST→语义检查→逻辑计划；错误应含类型/位置/原因。
- 操作系统模块：页分配/读写、缓冲池与 LRU、命中率统计与替换/写回日志。
- 数据库模块：Catalog 元数据、火山模型算子、持久化与重启后可查询。

## 开发计划（建议）
1) 完成 Frontend 最小子集（CREATE/INSERT/SELECT/DELETE）的 Lexer/Parser/Semantic/Plan。
2) 实现 Catalog（建表、查表、列定义），落盘到页式存储。
3) 实现 Engine 基础算子，串联 StorageEngine；补充 CLI REPL。
4) 扩展：UPDATE/JOIN/ORDER BY/GROUP BY、谓词下推、更多索引与优化。

## 版权与许可
本项目用于教学与学习目的。
