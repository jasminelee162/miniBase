#include "storage/storage_engine.h"
#include <iostream>
#include <cstring>

using namespace minidb;

int main() {
    StorageEngine engine("minidb.data", 64);
    page_id_t pid;
    Page* p = engine.CreatePage(&pid);
    if (!p) {
        std::cerr << "CreatePage failed" << std::endl;
        return 1;
    }
    const char* msg = "Hello MiniBase";
    std::memcpy(p->GetData(), msg, std::strlen(msg));
    engine.PutPage(pid, true);

    Page* r = engine.GetPage(pid);
    if (!r) {
        std::cerr << "GetPage failed" << std::endl;
        return 1;
    }
    std::cout << std::string(r->GetData(), r->GetData() + std::strlen(msg)) << std::endl;
    engine.PutPage(pid, false);

    engine.PrintStats();

    // Policy demo: switch to FIFO and touch many pages to trigger replacements
    engine.SetReplacementPolicy(ReplacementPolicy::FIFO);
    for (int i = 0; i < 200; ++i) {
        page_id_t tmp;
        Page* p2 = engine.CreatePage(&tmp);
        if (p2) {
            std::memset(p2->GetData(), 0xAB, 64);
            engine.PutPage(tmp, true);
        }
    }
    engine.Checkpoint();
    engine.PrintStats();
    engine.Shutdown();
    return 0;
}

