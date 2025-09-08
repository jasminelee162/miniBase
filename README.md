# miniBase

一个迷你关系型数据库教学实现，支持基本SQL解析、存储管理和查询执行。

## 功能特性
- 支持DDL/DML语句解析
- 页式存储管理（Buffer Pool + LRU）
- 火山模型查询执行引擎
- 系统目录管理（表/列元数据）

## 构建说明
```bash
mkdir build
cd build
cmake ..
make
```

## 架构图
![架构图](./docs/architecture.png)