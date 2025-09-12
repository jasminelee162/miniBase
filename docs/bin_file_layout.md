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
  - **页大小**：如 4096 字节。
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
