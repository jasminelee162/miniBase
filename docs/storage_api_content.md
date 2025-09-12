### Storage 模块快速上手（StorageEngine / Page / Buffer）

这份文档告诉你：怎么用存储模块完成“新建页、写入、读取、刷盘”，以及如何在执行引擎里对接使用。

---

## 1. 一句话认识
- StorageEngine 是入口：负责创建/获取/归还页面，刷盘，统计。
- Page 是 4KB 固定块（默认），通过 `GetData()` 直接读写字节。
- BufferPool 是内存缓存（单位是“页数”），避免频繁磁盘 I/O。
- 文件会按 `DEFAULT_DISK_SIZE_BYTES` 预分配（参见 `util/config.h`）。例如设置为 160KB，则约等于 40 页（`DEFAULT_MAX_PAGES = DEFAULT_DISK_SIZE_BYTES / PAGE_SIZE`）。

文件布局（.bin 单文件表空间）：
- 第 0 页：Meta Superblock，含魔数/版本/页大小/`next_page_id`/`catalog_root` 等；`page_type = METADATA_PAGE`。
- 第 1..N 页：数据/索引/系统页（由 `page_type` 区分），支持页链 `next_page_id`。

---

## 2. 最小可用示例（可直接复制）

### 基础页操作示例
```cpp
#include "storage/storage_engine.h"
#include <cstring>
using namespace minidb;

void quick_start() {
    // 第二个参数 128 是"页数"，不是字节。约等于 128 * 4096B 内存占用。
    StorageEngine engine("data/example.bin", /*buffer_pool_size=*/128);

    // 1) 创建新页并写入
    page_id_t pid; Page* p = engine.CreatePage(&pid);
    if (!p) return; // 可能是磁盘页数到达上限
    const char* msg = "Hello MiniDB";
    std::memcpy(p->GetData(), msg, std::strlen(msg));
    engine.PutPage(pid, /*is_dirty=*/true); // 写入后标脏

    // 2) 读取验证
    Page* r = engine.GetPage(pid);
    if (r) {
        // 比对数据...
        engine.PutPage(pid); // 读取后归还，不标脏
    }

    // 3) 可选：阶段性刷盘
    engine.Checkpoint();
}
```

### 元数据管理示例（新增）
```cpp
#include "storage/storage_engine.h"
using namespace minidb;

void metadata_example() {
    StorageEngine engine("data/metadata_example.bin");
    
    // 1) 初始化元数据页
    engine.InitializeMetaPage();
    
    // 2) 获取和更新元数据
    MetaInfo meta = engine.GetMetaInfo();
    std::cout << "Magic: 0x" << std::hex << meta.magic << std::endl;
    std::cout << "Next page ID: " << meta.next_page_id << std::endl;
    
    // 3) 创建目录页
    Page* catalog_page = engine.CreateCatalogPage();
    if (catalog_page) {
        std::cout << "Catalog page created: " << catalog_page->GetPageId() << std::endl;
        
        // 4) 更新目录数据
        std::vector<char> catalog_data = {'T', 'e', 's', 't', 'D', 'a', 't', 'a'};
        engine.UpdateCatalogData(CatalogData(catalog_data));
    }
    
    engine.Shutdown();
}
```

### B+树增强功能示例（新增）
```cpp
#include "storage/index/bplus_tree.h"
using namespace minidb;

void bplus_tree_example() {
    StorageEngine engine("data/bplus_example.bin");
    BPlusTree tree(&engine);
    
    // 1) 创建新树
    page_id_t root_id = tree.CreateNew();
    std::cout << "B+ tree root: " << root_id << std::endl;
    
    // 2) 插入数据
    RID rid1{1, 10}, rid2{2, 20}, rid3{3, 30};
    tree.Insert(100, rid1);
    tree.Insert(200, rid2);
    tree.Insert(150, rid3);
    
    // 3) 查找和更新
    auto result = tree.Search(100);
    if (result) {
        std::cout << "Found: page=" << result->page_id << ", slot=" << result->slot << std::endl;
    }
    
    // 4) 更新RID
    RID new_rid{99, 99};
    tree.Update(100, new_rid);
    
    // 5) 删除操作
    tree.Delete(150);
    
    // 6) 检查键存在性和统计
    std::cout << "Has key 100: " << tree.HasKey(100) << std::endl;
    std::cout << "Key count: " << tree.GetKeyCount() << std::endl;
    
    // 7) 范围查询
    auto range_results = tree.Range(100, 200);
    std::cout << "Range query results: " << range_results.size() << std::endl;
    
    // 8) 模板化操作（支持字符串键）
    tree.InsertGeneric<std::string>("hello", rid1);
    auto string_result = tree.SearchGeneric<std::string>("hello");
    if (string_result) {
        std::cout << "String key found!" << std::endl;
    }
    
    engine.Shutdown();
}
```

---

## 3. 你会用到的接口（90% 场景够用）

### 基础页操作
- 构造引擎：`StorageEngine(const std::string& db_file, size_t buffer_pool_size)`
- 页操作：
  - `Page* CreatePage(page_id_t* out_id)` 新页
  - `Page* GetPage(page_id_t id)` 取页
  - `bool PutPage(page_id_t id, bool is_dirty=false)` 归还（写了就 `true`）
  - `bool RemovePage(page_id_t id)` 删除页
- 批量：`std::vector<Page*> GetPages(const std::vector<page_id_t>& ids)`
- 页链接：`bool LinkPages(page_id_t from_page_id, page_id_t to_page_id)`
- 页链遍历：`std::vector<Page*> GetPageChain(page_id_t first_page_id)`
- 预取：`PrefetchPageChain(first_page_id, max_pages=8)`

### 元数据管理接口（新增）
- 元数据页操作：
  - `Page* GetMetaPage()` 获取第0页元数据页
  - `bool InitializeMetaPage()` 初始化元数据页
  - `MetaInfo GetMetaInfo() const` 获取元数据信息
  - `bool UpdateMetaInfo(const MetaInfo&)` 更新元数据
- 目录页操作：
  - `Page* GetCatalogPage()` 获取目录页
  - `Page* CreateCatalogPage()` 创建目录页
  - `bool UpdateCatalogData(const CatalogData&)` 更新目录页数据
- 元数据字段访问：
  - `page_id_t GetCatalogRoot() const` 获取catalog_root
  - `bool SetCatalogRoot(page_id_t)` 设置catalog_root
  - `page_id_t GetNextPageId() const` 获取next_page_id
  - `bool SetNextPageId(page_id_t)` 设置next_page_id

### 页内数据操作工具（新增）
- `bool AppendRecordToPage(Page* page, const void* record_data, uint16_t record_size)` 追加记录
- `std::vector<std::pair<const void*, uint16_t>> GetPageRecords(Page* page)` 获取页内所有记录
- `void InitializeDataPage(Page* page)` 初始化数据页

### 系统管理
- 刷盘：`void Checkpoint()`；关闭：`void Shutdown()`
- 统计：`double GetCacheHitRate() const`、`size_t GetNumReplacements() const`、`size_t GetNumWritebacks() const`
- 策略：`void SetReplacementPolicy(ReplacementPolicy)`（默认 LRU，可切 FIFO）
- 扩容：`bool AdjustBufferPoolSize(size_t new_size)`（当前支持增大）
- 后台刷盘：`StartBackgroundFlush(interval_ms)` / `StopBackgroundFlush()`

### B+树索引接口（增强）
- 基本操作：
  - `page_id_t CreateNew()` 创建新树
  - `void SetRoot(page_id_t root_id)` 设置根节点
  - `page_id_t GetRoot() const` 获取根节点
  - `bool Insert(int32_t key, const RID& rid)` 插入键值对
  - `std::optional<RID> Search(int32_t key)` 查找键
  - `std::vector<RID> Range(int32_t low, int32_t high)` 范围查询
- 增强操作（新增）：
  - `bool Delete(int32_t key)` 删除键值对
  - `bool Update(int32_t key, const RID& new_rid)` 更新RID
  - `bool HasKey(int32_t key)` 检查键是否存在
  - `size_t GetKeyCount() const` 获取键数量
- 模板化操作（新增）：
  - `template<typename KeyType> bool InsertGeneric(const KeyType& key, const RID& rid)`
  - `template<typename KeyType> std::optional<RID> SearchGeneric(const KeyType& key)`
  - `template<typename KeyType> bool DeleteGeneric(const KeyType& key)`

Page（页）：
- `char* GetData()` 取 4KB 数据缓冲
- `bool IsDirty()/SetDirty(bool)` 脏标
- `int GetPinCount()` 引用计数（一般不用手动改）

---

## 4. 容量与常见坑
- 每页 4096B（默认）。文件预分配大小由 `DEFAULT_DISK_SIZE_BYTES` 决定（见 `util/config.h`），最大页数为 `DEFAULT_MAX_PAGES = DEFAULT_DISK_SIZE_BYTES / PAGE_SIZE`。
- 超过最大页数：`CreatePage` 返回 `nullptr`。并发/压力测试请优先基于配置计算：
  - `size_t cap = DEFAULT_MAX_PAGES;`
  - 规模或断言用 `min(期望页数, cap)`，避免配置变化导致失败。
- 缓冲池大小的单位是“页数”：例如 128 表示最多缓存 128 页。
- 页 0 是元数据页（`METADATA_PAGE`），不要当作数据页遍历；数据页从 1 开始。
- 若切换/重启：`Checkpoint()` 会持久化页 0 的 `next_page_id` 等，避免“扫描推断”误差。
- `GetPage(id)` 对未分配页（`id >= GetNumPages()`）会返回 `nullptr`；遍历链请处理空指针与环路。

---

## 5. 执行引擎对接模板
- 追加一行（自动处理“页满则换新页”）：
```cpp
page_id_t cur = table.first_page_id; // 无则置 INVALID_PAGE_ID
for (;;) {
    if (cur == INVALID_PAGE_ID) {
        Page* np = engine.CreatePage(&cur);
        // 在 Catalog/目录页登记 cur；初始化页头
        engine.PutPage(cur, true);
    }
    Page* p = engine.GetPage(cur);
    bool ok = append_row(p, row_data, row_len); // 页内附带槽目录/页头逻辑由上层实现
    engine.PutPage(cur, ok);
    if (ok) break;
    // 分配新页并链接
    page_id_t next; Page* np = engine.CreatePage(&next);
    link_pages(cur, next); // 在你的目录结构里记录链表关系
    engine.PutPage(next, true);
    cur = next;
}
```
- 顺序扫描：
```cpp
for (page_id_t id = table.first_page_id; id != INVALID_PAGE_ID; id = next_of(id)) {
    Page* p = engine.GetPage(id);
    for_each_row(p, [&](const Row& r){ /* 过滤/投影/输出 */ });
    engine.PutPage(id);
}
```
- 顺序扫描前预取（可选）：
```cpp
engine.PrefetchPageChain(table.first_page_id, 16);
```

---

## 6. 构建集成（CMake）
```cmake
add_executable(myexec main.cpp)
target_link_libraries(myexec PRIVATE storage_lib util_lib)
# include 路径通常由上层 CMake 提供；如需手动：
# target_include_directories(myexec PRIVATE ${CMAKE_SOURCE_DIR}/src)
```

---

## 7. 最后检查（Checklist）
- 引入 `storage/storage_engine.h` 并链接 `storage_lib`/`util_lib`
- 设计页内结构（页头、槽目录、记录序列化）
- 用 `CreatePage/GetPage/PutPage` 完成写入/读取
- 并发/批量按 `DEFAULT_MAX_PAGES` 控制规模
- 关键路径后调用 `Checkpoint()`；或开启 `StartBackgroundFlush(..)`；结束 `Shutdown()`
- 用 `PrintStats()`/`GetCacheHitRate()` 观察命中与写回

---

## 8. 文件与元数据（Meta Superblock）

### 文件布局
- 单库单文件：建议放在 `data/<db>.bin`。页 0 为 Superblock：
  - `magic, version, page_size, next_page_id, catalog_root`（预留）
  - 程序启动读取；`Checkpoint/Shutdown` 持久化；`next_page_id` 从 1 起（保留页 0）。
- 数据/索引/系统页类型通过 `PageHeader.page_type` 区分；数据页支持 `next_page_id` 链式遍历。

### 元数据结构（新增）
```cpp
struct MetaInfo {
    uint64_t magic;           // 魔数标识 "MiniDB_M" (0x4D696E6944425F4D)
    uint32_t version;         // 版本号 (当前为1)
    uint32_t page_size;       // 页大小 (4096)
    page_id_t next_page_id;   // 下一个可分配页号
    page_id_t catalog_root;   // 目录页根页号
};
```

### 页类型枚举（新增）
```cpp
enum class PageType : uint32_t {
    DATA_PAGE = 0,      // 数据页
    INDEX_PAGE = 1,     // 索引页
    METADATA_PAGE = 2,  // 元数据页
    CATALOG_PAGE = 3    // 目录页
};
```

### 元数据管理最佳实践
- 启动时调用 `InitializeMetaPage()` 初始化元数据页
- 使用 `GetMetaInfo()` 和 `UpdateMetaInfo()` 管理元数据
- 通过 `CreateCatalogPage()` 和 `UpdateCatalogData()` 管理目录页
- 定期调用 `Checkpoint()` 确保元数据持久化

---

## 9. 可靠性与健壮性
- 脏页管理：页头/记录写入会置脏；`Checkpoint` 与后台线程负责刷盘。
- 链路安全：`GetPageChain` 内建环路保护；非法页返回空。
- 重启恢复：依赖页 0 的 meta，避免误判容量/已用页数。

## 10. 功能增强对比（新增）

| 功能模块 | 增强前 | 增强后 |
|----------|--------|--------|
| **元数据管理** | ❌ 无接口 | ✅ 完整接口 |
| **目录页管理** | ❌ 无接口 | ✅ 完整接口 |
| **B+树删除** | ❌ 不支持 | ✅ 支持 |
| **B+树更新** | ❌ 不支持 | ✅ 支持 |
| **多键类型** | ❌ 仅int32 | ✅ 模板化支持 |
| **页内数据操作** | ⚠️ 手动实现 | ✅ 便利工具 |
| **元数据持久化** | ⚠️ 基础支持 | ✅ 增强支持 |

### 新增特性亮点
- **完整的元数据管理**：支持第0页Meta Superblock的完整生命周期管理
- **目录页支持**：为未来Catalog模块迁移到页式存储做好准备
- **B+树CRUD**：支持完整的增删改查操作，包括模板化多键类型支持
- **便利工具**：提供页内数据操作的便利接口，减少重复代码
- **向后兼容**：所有新功能都保持与现有代码的完全兼容性


## 附：可直接拷贝的页内布局与工具（最小版）

说明：以下示例仅基于已有常量与类型（PAGE_SIZE、page_id_t、INVALID_PAGE_ID）。不依赖项目其余私有结构，可直接复制到你的执行引擎/工具文件中使用。

```cpp
// 可放入 src/exec/page_utils.h 或相关位置
#pragma once
#include "util/config.h"
#include "storage/page/page.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include <utility>

namespace minidb {

// 简化页头（放在页首）
struct SimplePageHeader {
    uint16_t slot_count;        // 已用槽数
    uint16_t free_space_offset; // 自页首起的可用空间偏移
    uint32_t next_page_id;      // 可选：链表指针（无则设为 INVALID_PAGE_ID）
};

// 槽目录条目（固定 4 字节）：[offset(2B), length(2B)]，从页尾向上增长
struct SlotEntry { uint16_t offset; uint16_t length; };

// 布局（以 PAGE_SIZE=4096 为例）：
// [SimplePageHeader |   ...记录区向下增长...   | ...空闲... | ...槽目录向上增长... ]
// 记录写入 data + hdr.free_space_offset，槽目录写在页尾（倒序）。

inline SimplePageHeader* get_header(Page* page) {
    return reinterpret_cast<SimplePageHeader*>(page->GetData());
}

inline SlotEntry* get_slot(Page* page, uint16_t slot_index) {
    auto* base = reinterpret_cast<unsigned char*>(page->GetData());
    // 槽 i 位于：页尾起始位置 - (i+1)*sizeof(SlotEntry)
    size_t slot_bytes = sizeof(SlotEntry) * (static_cast<size_t>(slot_index) + 1);
    return reinterpret_cast<SlotEntry*>(base + PAGE_SIZE - slot_bytes);
}

inline uint16_t header_size() { return static_cast<uint16_t>(sizeof(SimplePageHeader)); }
inline uint16_t slot_size() { return static_cast<uint16_t>(sizeof(SlotEntry)); }

// 初始化新页（在 CreatePage 后调用一次）
inline void init_page(Page* page) {
    auto* hdr = get_header(page);
    hdr->slot_count = 0;
    hdr->free_space_offset = header_size();
    hdr->next_page_id = static_cast<uint32_t>(INVALID_PAGE_ID);
}

// 计算当前剩余可用空间（不含槽目录本身）
inline uint16_t free_space(const Page* page) {
    auto* data = const_cast<char*>(page->GetData());
    auto* hdr = reinterpret_cast<const SimplePageHeader*>(data);
    uint16_t used_for_slots = static_cast<uint16_t>(hdr->slot_count * slot_size());
    // 槽目录占据页尾的 used_for_slots 字节
    return static_cast<uint16_t>(PAGE_SIZE - used_for_slots - hdr->free_space_offset);
}

// 追加一条记录；成功返回 true，并在 out_slot 返回槽序号
inline bool append_row(Page* page, const void* row, uint16_t len, uint16_t* out_slot = nullptr) {
    auto* data = reinterpret_cast<unsigned char*>(page->GetData());
    auto* hdr = get_header(page);

    // 需要的空间：记录 len + 新增一个槽条目
    uint16_t need = static_cast<uint16_t>(len + slot_size());
    if (free_space(page) < need) return false;

    // 写入记录
    uint16_t write_off = hdr->free_space_offset;
    std::memcpy(data + write_off, row, len);
    hdr->free_space_offset = static_cast<uint16_t>(write_off + len);

    // 写入槽目录（页尾）
    SlotEntry* slot = get_slot(page, hdr->slot_count);
    slot->offset = write_off;
    slot->length = len;
    if (out_slot) *out_slot = hdr->slot_count;
    hdr->slot_count = static_cast<uint16_t>(hdr->slot_count + 1);
    return true;
}

// 遍历所有记录（回调接收每条记录的起始指针与长度）
inline void for_each_row(Page* page, const std::function<void(const unsigned char*, uint16_t)>& fn) {
    auto* data = reinterpret_cast<unsigned char*>(page->GetData());
    auto* hdr = get_header(page);
    for (uint16_t i = 0; i < hdr->slot_count; ++i) {
        const SlotEntry* s = get_slot(page, i);
        fn(data + s->offset, s->length);
    }
}

// 简单获取下一页 id（如未使用链表，可忽略）
inline page_id_t next_of(Page* page) {
    auto* hdr = get_header(page);
    return static_cast<page_id_t>(hdr->next_page_id);
}

// 简单设置下一页 id（在创建新页并链接时调用）
inline void set_next(Page* page, page_id_t next) {
    auto* hdr = get_header(page);
    hdr->next_page_id = static_cast<uint32_t>(next);
}

} // namespace minidb
```

用法要点：
- 新页创建后，先 `init_page(p)` 初始化，再 `append_row` 写入；
- 扫描时用 `for_each_row(p, ...)`；
- 需要页链表时，用 `set_next/next_of` 维护；
- 写入/修改后，`engine.PutPage(id, true)` 标脏归还；只读后 `engine.PutPage(id)`。
