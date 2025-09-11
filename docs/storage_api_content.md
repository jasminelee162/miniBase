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
```cpp
#include "storage/storage_engine.h"
#include <cstring>
using namespace minidb;

void quick_start() {
    // 第二个参数 128 是“页数”，不是字节。约等于 128 * 4096B 内存占用。
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

---

## 3. 你会用到的接口（90% 场景够用）
- 构造引擎：`StorageEngine(const std::string& db_file, size_t buffer_pool_size)`
- 页操作：
  - `Page* CreatePage(page_id_t* out_id)` 新页
  - `Page* GetPage(page_id_t id)` 取页
  - `bool PutPage(page_id_t id, bool is_dirty=false)` 归还（写了就 `true`）
  - `bool RemovePage(page_id_t id)` 删除页
- 批量：`std::vector<Page*> GetPages(const std::vector<page_id_t>& ids)`
- 刷盘：`void Checkpoint()`；关闭：`void Shutdown()`
- 统计：`double GetCacheHitRate() const`、`size_t GetNumReplacements() const`、`size_t GetNumWritebacks() const`
- 策略：`void SetReplacementPolicy(ReplacementPolicy)`（默认 LRU，可切 FIFO）
- 扩容：`bool AdjustBufferPoolSize(size_t new_size)`（当前支持增大）
- 后台刷盘：`StartBackgroundFlush(interval_ms)` / `StopBackgroundFlush()`
- 预取：`PrefetchPageChain(first_page_id, max_pages=8)`

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
- 单库单文件：建议放在 `data/<db>.bin`。页 0 为 Superblock：
  - `magic, version, page_size, next_page_id, catalog_root`（预留）
  - 程序启动读取；`Checkpoint/Shutdown` 持久化；`next_page_id` 从 1 起（保留页 0）。
- 数据/索引/系统页类型通过 `PageHeader.page_type` 区分；数据页支持 `next_page_id` 链式遍历。

---

## 9. 可靠性与健壮性
- 脏页管理：页头/记录写入会置脏；`Checkpoint` 与后台线程负责刷盘。
- 链路安全：`GetPageChain` 内建环路保护；非法页返回空。
- 重启恢复：依赖页 0 的 meta，避免误判容量/已用页数。


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
