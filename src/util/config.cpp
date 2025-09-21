#include "util/config.h"

namespace minidb {

static RuntimeConfig g_cfg;

RuntimeConfig& GetRuntimeConfig() {
    // 固定使用编译期默认值（config.h 中定义），不再读取环境变量覆盖。
    return g_cfg;
}

}


