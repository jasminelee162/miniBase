// 使用 SimpleTest 框架
#include "../simple_test_framework.h"
#include "util/config.h"
#include "storage/storage_engine.h"
#include <string>

using namespace minidb;
using namespace SimpleTest;

static void tc_runtime_config_defaults() {
    RuntimeConfig &cfg = GetRuntimeConfig();
    ASSERT_EQ((size_t)BUFFER_POOL_SIZE, cfg.buffer_pool_pages);
    StorageEngine se("data/test_runtime.db", 0);
    ASSERT_EQ((size_t)BUFFER_POOL_SIZE, se.GetBufferPoolSize());
}

static void tc_metrics_basic() {
    StorageEngine se("data/test_metrics.db", 32);
    double hr = se.GetCacheHitRate();
    ASSERT_TRUE(hr >= 0.0);
    (void)se.GetNumReplacements();
    (void)se.GetNumWritebacks();
    (void)se.GetIOQueueDepth();
}

int main(){
    TestSuite suite;
    suite.addTest("runtime_config_defaults", tc_runtime_config_defaults);
    suite.addTest("metrics_basic", tc_metrics_basic);
    suite.runAll();
    return TestCase::getFailed() == 0 ? 0 : 1;
}


