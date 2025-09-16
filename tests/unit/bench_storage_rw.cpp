#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include "../../src/storage/storage_engine.h"

using namespace minidb;

int main(){
    StorageEngine se("data/bench_storage_rw.db", 256);
    const int threads = 8;
    const int ops_per_thread = 100;

    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> th;
    for (int ti=0; ti<threads; ++ti){
        th.emplace_back([&](){
            for (int i=0;i<ops_per_thread;++i){
                page_id_t pid = INVALID_PAGE_ID;
                Page* p = se.CreateDataPage(&pid);
                if (!p) continue;
                const char* msg = "bench";
                AppendRow(p, reinterpret_cast<const unsigned char*>(msg), (uint16_t)strlen(msg));
                se.PutPage(pid, true);
            }
        });
    }
    for (auto& x: th) x.join();
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    size_t total_ops = static_cast<size_t>(threads) * static_cast<size_t>(ops_per_thread);
    double tput = (double)total_ops / (ms/1000.0);
    std::cout << "ops=" << total_ops << ", time_ms=" << ms << ", throughput_ops_per_sec=" << (size_t)tput << std::endl;
    return 0;
}


