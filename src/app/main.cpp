#include "yw/core.h"
#include "yw/node.h"
#include "yw/app_context.h"
#include <spdlog/spdlog.h>

// 组装应用
int main() {
    spdlog::info("Starting yw application...");

    std::shared_ptr<yw::core::AppContext> app_context = std::make_shared<yw::core::AppContext>();
    if (!app_context->initialize()) {
        spdlog::error("Failed to initialize app context");
        return 1;
    }

    std::shared_ptr<yw::node::INodeModule> node_module = yw::node::NodeFactory::createNodeModule(app_context->getHttpServer());
    
    // 使用核心模块的工具函数
    yw::utils::print_hello();

    app_context->runHttpServer();

    spdlog::info("Application finished.");

    return 0;
}