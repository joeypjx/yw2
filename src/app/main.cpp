#include "yw/core.h"
#include <spdlog/spdlog.h>

// 组装应用
int main() {
    spdlog::info("Starting yw application...");
    
    // 使用核心模块的工具函数
    yw::utils::print_hello();
    
    spdlog::info("Application finished.");
    return 0;
}