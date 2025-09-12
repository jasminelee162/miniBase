# MiniBase 数据库磁盘文件（.bin）结构说明

本文件说明 MiniBase 数据库的 `.bin` 文件（单文件表空间）在磁盘上的布局、各页的作用，以及表结构元数据的存储方式。

---

## 文件整体布局

- 所有数据、元数据、索引等都存储在一个 `.bin` 文件中。
- 文件空间在创建时按 `DEFAULT_DISK_SIZE_BYTES` 预分配（详见 `util/config.h`）。

---

## 页类型与分布

### 1. 第 0 页：元数据页（Meta Superblock）

- 作用：保存数据库文件的基础元信息。
- 内容包括：
  - **魔数** `"M_BDiniM"`：用于标识文件类型和合法性。
  - **版本号**：数据库文件格式版本。
  - **页大小**：如 4096 字节。4KB
  - **next_page_id**：下一个可分配页号。
  - **catalog_root**：目录页（Catalog Page）的页号入口。
  - 其他元数据字段。
- `page_type` 通常为 `METADATA_PAGE`。

### 2. 目录页（Catalog Page）

- 由第 0 页的 `catalog_root` 字段指向。
- 作用：保存所有表的元数据（表名、字段名、字段类型等）。
- 本质上也是一个普通页，只是 `page_type` 为 `CATALOG_PAGE`。
- 典型结构体如 `TableMeta` 可序列化到目录页中，描述表结构。

### 3. 数据页（Data Page）

- 从第 1 页开始，或由目录页分配。
- 作用：保存表的实际数据行。
- `page_type` 为 `DATA_PAGE`。
- 只存储数据，不保存表结构信息。

### 4. 索引页（Index Page）

- 用于存储索引结构（如 B+树节点）。
- `page_type` 为 `INDEX_PAGE`。

---

## 典型访问流程

1. **启动时**，读取第 0 页，校验魔数，获取 `catalog_root`。
2. **读取表结构**，通过 `catalog_root` 定位目录页，解析表名、字段名、类型等元数据。
3. **访问数据**，根据目录页信息定位数据页，读取或写入实际数据行。
4. **索引操作**，通过索引页加速数据检索。

---

## 设计总结

- **第 0 页**：保存文件元数据和入口信息（如 `catalog_root`）。
- **目录页**：保存所有表的结构元数据，由第 0 页指向。
- **数据页**：保存表的实际数据行。
- **索引页**：保存索引结构。
- 页类型通过 `page_type` 字段区分，便于扩展和管理。
---

## 存储系统（StorageEngine）与 B+ 树索引（BPlusTree）

### 存储系统职责与接口

- 统一页 API：`CreatePage/GetPage/PutPage/DeletePage/LinkPages/GetPages`
- 元数据页：`InitializeMetaPage/GetMetaInfo/UpdateMetaInfo`
- 目录页：`CreateCatalogPage/GetCatalogPage/UpdateCatalogData`
- 数据页便捷：`CreateDataPage/GetDataPage/InitializeDataPage/AppendRecordToPage/GetPageRecords`
- 索引页便捷：`CreateIndexPage/GetIndexPage/InitializeIndexPage`
- 统计/调优：`PrintStats/GetCacheHitRate/SetReplacementPolicy/AdjustBufferPoolSize`
- 索引根持久化：`GetIndexRoot/SetIndexRoot`（写在 Meta.reserved 字节区）

说明：所有页 I/O 均经由缓冲池，自动应用页替换策略与命中率统计，无需对不同页类型做额外处理。

### B+ 树职责与接口

- 构造：`BPlusTree(StorageEngine*)`
- 根管理：`LoadRootFromStorage()`（加载已保存根）/`CreateNew()`（新建并持久化根）/`SetRoot()`（设置并持久化根）
- 操作：`Insert/Search/Range/Delete`（已实现多层分裂、范围扫描、删除重平衡与根降级）
- 节点布局：PageHeader 后是 `NodeHeader`，叶子存 `LeafEntry[]`，内节点提供 `children[]/keys[]` 视图；叶子 `prev/next` 便于范围查询。

---

## Catalog（目录）如何对接

- Catalog 负责记录表与索引的语义信息（表名、列名、类型、哪些列建索引、索引名称等），并序列化到目录页链。StorageEngine 提供目录页的创建/读取/写入，格式由上层定义。
- 索引根页号：
  - 简化方案（已实现）：`StorageEngine::SetIndexRoot/GetIndexRoot` 将一个索引根写入 Meta.reserved；`BPlusTree::CreateNew/SetRoot` 自动调用。
  - 多索引方案：将“索引名 → 根页号”的映射序列化到目录页链（用 `UpdateCatalogData`），启动时 Catalog 读出映射并为每个索引构造 `BPlusTree`。

---

## 数据库系统（执行器）如何对接

最小接线：

1) 启动
```cpp
StorageEngine engine(db_path);
BPlusTree bpt(&engine);
bpt.LoadRootFromStorage();
if (bpt.GetRoot() == INVALID_PAGE_ID) bpt.CreateNew();
```

2) 插入（INSERT）
```cpp
// 写数据页获得 RID（page_id, slot）
// 从行构造 key
bpt.Insert(key, rid); // 多索引则逐个调用
```

3) 查询（SELECT）
```cpp
auto r = bpt.Search(key);           // 等值
auto rs = bpt.Range(low, high);     // 范围
// 对 RID 回表读取数据页（StorageEngine::GetPage）
```

4) 删除/更新
```cpp
bpt.Delete(key);
// 更新键：bpt.Delete(oldKey); bpt.Insert(newKey, rid);
```

说明：缓冲、替换、刷盘由 StorageEngine/BufferPool 自动处理；`PutPage(pid, true)` 标脏后，`Checkpoint()` 或后台线程将落盘。

---

## 端到端流程小结

- CreateTable：Catalog 序列化表结构到目录页链（Storage 负责写入）。
- CreateIndex：构造 `BPlusTree`，`LoadRootFromStorage()`，必要时 `CreateNew()`；Catalog 记录索引语义。
- Insert/Select/Delete：执行器根据计划调用 `BPlusTree` 的 `Insert/Search/Range/Delete`；回表通过 `StorageEngine` 页 API 读取行。

通过以上接线，项目已具备：
- 存储：Meta/Catalog/Data/Index 页的统一管理与 I/O；
- 索引：B+ 树插入/查找/范围/删除（含多层分裂与重平衡）与根持久化；
- 对接：目录页用于记录语义与多索引映射，执行器按计划调用索引与存储即可完成端到端数据路径。

## Storage 模块页访问接口（易用 API）

以下接口均定义在 `src/storage/storage_engine.h`，仅依赖 storage 模块：

- 元数据页（第 0 页）
  - `Page* GetMetaPage()`：获取元数据页指针。
  - `bool InitializeMetaPage()`：初始化元数据页（写入魔数、版本等）。
  - `MetaInfo GetMetaInfo() const` / `bool UpdateMetaInfo(const MetaInfo&)`：获取/更新元数据信息。
  - 字段访问：`GetCatalogRoot() / SetCatalogRoot(...)`，`GetNextPageId() / SetNextPageId(...)`。

- 目录页（Catalog Page）
  - `Page* CreateCatalogPage()`：创建并初始化目录页，自动更新 `catalog_root`。
  - `Page* GetCatalogPage()`：按 `catalog_root` 获取目录页。
  - `bool UpdateCatalogData(const CatalogData&)`：将序列化后的目录数据写入目录页。

- 数据页（Data Page）
  - `void InitializeDataPage(Page* page)`：将任意页初始化为数据页。
  - `Page* CreateDataPage(page_id_t* page_id)`：创建并初始化数据页，返回可用页指针。
  - `Page* GetDataPage(page_id_t page_id)`：按页号获取数据页（校验 `page_type`）。
  - 记录操作：`bool AppendRecordToPage(Page*, const void* data, uint16_t len)`、`std::vector<std::pair<const void*, uint16_t>> GetPageRecords(Page*)`。

- 索引页（Index Page）
  - `void InitializeIndexPage(Page* page)`：将任意页初始化为索引页。
  - `Page* CreateIndexPage(page_id_t* page_id)`：创建并初始化索引页。
  - `Page* GetIndexPage(page_id_t page_id)`：按页号获取索引页（校验 `page_type`）。

---

## 示例代码（调用方法）

```cpp
#include "storage/storage_engine.h"
using namespace minidb;

void example(StorageEngine& engine) {
    // 1) 元数据页
    engine.InitializeMetaPage();
    MetaInfo m = engine.GetMetaInfo();
    m.next_page_id = 10;
    engine.UpdateMetaInfo(m);

    // 2) 目录页
    Page* cat = engine.CreateCatalogPage();
    std::vector<char> bytes = {'T','a','b','l','e'};
    engine.UpdateCatalogData(CatalogData(bytes));
    engine.PutPage(cat->GetPageId(), true);

    // 3) 数据页
    page_id_t dpid;
    Page* data = engine.CreateDataPage(&dpid);
    const char* msg = "hello";
    engine.AppendRecordToPage(data, msg, 5);
    auto recs = engine.GetPageRecords(data);
    engine.PutPage(dpid, true);

    // 4) 索引页
    page_id_t ipid;
    Page* index = engine.CreateIndexPage(&ipid);
    // 写入索引节点自定义结构...
    engine.PutPage(ipid, true);
}
```

---

## 插入一条数据：访问路径与接口配合

下面以“插入一条数据”为例，按访问路径说明 Storage 的职责与接口配合方式（不区分上层解析/执行器细节）：

- 第0步（前置信息）
  - 已有库文件（.bin）与元数据页 page 0；若是新文件，先调用 `InitializeMetaPage()`。
  - 目录页存在且记录了该表的数据页链入口；若首次创建表，通常会先 `CreateCatalogPage()` 并写入目录元数据。

- 第1步：定位数据页
  - 从目录中获取该表的数据页链入口（例如首个数据页 page_id）。
  - 若该表还没有数据页：调用 `CreateDataPage(&pid)` 创建第一页（内部已初始化为 DATA_PAGE）。

- 第2步：将目标页加载到缓冲池
  - `Page* p = storage.GetDataPage(target_pid);`
  - 这一步经过 BufferPoolManager，自动应用页替换策略和缓存统计。

- 第3步：向页追加记录
  - 调用 `AppendRecordToPage(p, record_data, record_len)` 尝试写入。
  - 若剩余空间不足：
    - 创建新数据页：`CreateDataPage(&new_pid)`；
    - 用 `LinkPages(old_pid, new_pid)` 把老页串到新页；
    - 切换到新页继续 `AppendRecordToPage(...)`。

- 第4步：落盘与归还
  - 标记写回：`PutPage(pid, true)`（脏页为 true）；
  - 可选：触发落盘或检查点 `Checkpoint()`；否则由缓冲池与后台刷新线程按策略写回。

- 第5步：维护索引（如有）
  - 对每个索引（B+树）：
    - 读取/创建相关索引页：`GetIndexPage(...)` / `CreateIndexPage(...)`；
    - 在 B+ 树中插入键（可能触发节点分裂，访问多个索引页，均通过缓冲池管理）；
    - 对被修改的索引页调用 `PutPage(page_id, true)`。

- 第6步：统计与调优（可选）
  - 查看命中率/淘汰/写回：`PrintStats()`、`GetCacheHitRate()`、`GetNumReplacements()`、`GetNumWritebacks()`。
  - 顺序或链式写入前可 `PrefetchPageChain(first_pid)` 提前提高命中率。
  - 调整策略或容量：`SetReplacementPolicy(...)`、`AdjustBufferPoolSize(...)`。

简要示例（仅演示存储层调用顺序）
```cpp
using namespace minidb;

void insert_one(StorageEngine& engine, const void* data, uint16_t len, page_id_t head_pid) {
    // 1) 若无页则创建第一页
    page_id_t pid = head_pid;
    Page* p = pid == INVALID_PAGE_ID ? engine.CreateDataPage(&pid) : engine.GetDataPage(pid);
    if (!p) return;

    // 2) 追加，不够则扩展链
    if (!engine.AppendRecordToPage(p, data, len)) {
        engine.PutPage(pid, false);
        page_id_t new_pid;
        Page* np = engine.CreateDataPage(&new_pid);
        if (!np) return;
        engine.LinkPages(pid, new_pid);
        engine.AppendRecordToPage(np, data, len);
        engine.PutPage(new_pid, true);
    } else {
        engine.PutPage(pid, true);
    }

    // 3) 如需强制持久化元数据或页头更新：engine.Checkpoint();
}
```

要点总结
- 不同页类型无需特殊缓存逻辑，统一经过缓冲池；替换策略与命中统计自动生效。
- 数据页关键接口：`CreateDataPage`、`GetDataPage`、`AppendRecordToPage`、`PutPage`、`LinkPages`。
- 索引页关键接口：`CreateIndexPage`、`GetIndexPage`，具体节点操作由索引模块实现。
- 目录/元数据页用于记录入口与全局信息（`CreateCatalogPage`、`GetCatalogPage`、`Get/UpdateMetaInfo`），插入路径中主要用于定位数据页链的起点。


四类页的讲解：

### 页0：元数据页（Meta Superblock）
- 职责
  - 标识并校验数据库文件：魔数、版本、页大小。
  - 提供入口信息：`next_page_id`（下一可分配页）、`catalog_root`（目录页入口）。
- 管理机制
  - 物理位置固定为第0页，`page_type = METADATA_PAGE`。
  - 通过 `DiskManager::ReadMeta/WriteMeta` 严格覆盖式读写，原子更新缓存的 Meta。
  - `StorageEngine` 对外提供字段化访问：`Get/SetNextPageId`、`Get/SetCatalogRoot`、`Get/UpdateMetaInfo`。
- 真实使用流程（例：初始化数据库）
  - 新建库文件 → `InitializeMetaPage()` 写入魔数/版本/页大小，设置 `next_page_id = 1`、`catalog_root = INVALID_PAGE_ID`。
  - 任何创建新页时，底层分配成功后推进 `next_page_id`；检查点或关闭时 `PersistMeta()`。

### 目录页（Catalog Page）
- 职责
  - 存放“表级元数据”：表名、字段名、类型、长度、索引信息、数据页链入口等（序列化结构，如 `TableMeta`）。
- 管理机制
  - `page_type = CATALOG_PAGE`，其页号被写入 Meta 的 `catalog_root` 字段。
  - `StorageEngine` 提供：`CreateCatalogPage()`（创建并设置 `catalog_root`）、`GetCatalogPage()`、`UpdateCatalogData(...)`（把序列化 bytes 写入页）。
  - 目录内容的具体结构与解释由上层模块（例如 Catalog 组件）负责；Storage 只负责“页的存取与持久化”。
- 真实使用流程（例：创建表）
  - 解析 SQL → 由上层生成 `TableMeta` → 序列化为二进制 → 调用 `UpdateCatalogData(bytes)` 写入目录页。
  - 若首次创建目录页：`CreateCatalogPage()`，内部初始化页头、更新 `catalog_root`，再写入 bytes。

### 数据页（Data Page）
- 职责
  - 存储表的“行记录”或变体（定长/变长记录）。页头维护槽信息、空闲空间指针以及 `next_page_id` 用于页链。
- 管理机制
  - `page_type = DATA_PAGE`；统一通过缓冲池 `BufferPoolManager` 进行 Fetch/New/Unpin。
  - 记录管理由 `page_utils.h` 的工具函数实现（如 `AppendRow`、`ForEachRow`），Storage 对外包装：
    - `CreateDataPage(&pid)`、`GetDataPage(pid)`、`InitializeDataPage(page)`、`AppendRecordToPage(page, data, len)`、`GetPageRecords(page)`。
  - 页满时通过 `LinkPages(from, to)` 把新页接到当前页尾，形成数据页链。
- 真实使用流程（例：插入一条记录）
  - 通过目录信息找到该表当前写入页（或其链表头）。
  - `GetDataPage(pid)` → `AppendRecordToPage(p, record, len)`：
    - 成功：`PutPage(pid, true)` 标脏归还。
    - 失败（空间不足）：`CreateDataPage(&new_pid)` → `LinkPages(pid, new_pid)` → 在新页 `AppendRecordToPage(...)` → `PutPage(new_pid, true)`。
  - 顺序批量写可配合 `PrefetchPageChain(first_pid)` 提高命中率。

### 索引页（Index Page）
- 职责
  - 存放索引结构节点（如 B+ 树的内部/叶子节点），用于加速点查/范围查。
- 管理机制
  - `page_type = INDEX_PAGE`；由索引模块（比如 B+Tree 实现）负责节点布局与分裂/合并策略。
  - Storage 只提供页级原语：`CreateIndexPage(&pid)`、`GetIndexPage(pid)`、`InitializeIndexPage(page)`、`PutPage(pid, dirty)`；其余由索引算法组织。
- 真实使用流程（例：插入键）
  - 执行器发起插入 → 索引模块自顶向下定位插入位置，沿途 `GetIndexPage(node_pid)`。
  - 需要新节点时：`CreateIndexPage(&new_pid)` → 上层在节点中写入键/指针结构 → `PutPage(new_pid, true)`。
  - 节点分裂：新页创建、父节点更新（可能级联写多个索引页），全部通过缓冲池读写与回写。

### 横切关注点（四类页通用）
- 缓存与替换策略：所有读写都经过 `BufferPoolManager`，自动统计命中率与替换次数；无需为不同页类型写特殊缓存代码。
- 同步与持久化：
  - 修改完页内容后 `PutPage(pid, true)` 标脏；`Checkpoint()` 或后台线程触发刷盘。
  - 关闭 `StorageEngine` 时，会 `FlushAllPages()` 并 `PersistMeta()`。
- 观测与调优接口：
  - 指标：`PrintStats()`、`GetCacheHitRate()`、`GetNumReplacements()`、`GetNumWritebacks()`。
  - 策略/容量：`SetReplacementPolicy(...)`、`AdjustBufferPoolSize(...)`。
  - 访问模式优化：顺序或链式访问时用 `PrefetchPageChain(...)`。

这套职责划分保证了：
- Storage 专注“页”的创建、类型初始化、链接、缓冲与持久化；
- Catalog/索引/执行器分别定义结构与策略；
- 四类页的访问路径统一、可观测、可调优，便于逐步扩展至更完整的数据库系统。
