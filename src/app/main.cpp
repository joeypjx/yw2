#include "yw/core.h"
#include "yw/node.h"
#include "yw/monitor.h"
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

    // 启动http服务
    app_context->runHttpServer();

    // 创建节点模块
    std::shared_ptr<yw::node::INodeModule> node_module = yw::node::NodeFactory::getNodeModule(app_context->getHttpService());
    std::shared_ptr<yw::monitor::IMonitorModule> monitor_module = yw::monitor::MonitorFactory::getMonitorModule(app_context->getHttpService());

    // 使用核心模块的工具函数
    yw::utils::print_hello();

    std::this_thread::sleep_for(std::chrono::seconds(1000));

    std::vector<yw::node::NodeExt> nodes = node_module->getAllNodes();
    for (const auto& node : nodes) {
        spdlog::info("Node: {}", node.host_ip);
    }

    spdlog::info("Application finished.");

    return 0;
}