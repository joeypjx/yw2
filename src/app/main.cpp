#include <spdlog/spdlog.h>
#include "yw/core.h"
#include "yw/node.h"
#include "yw/monitor.h"
#include "yw/web.h"
#include "yw/bmc.h"
#include "yw/alert.h"
#include "yw/app_context.h"

#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>

static std::atomic<bool> g_running{true};

static void handle_signal(int) {
    g_running = false;
}

// 组装应用
int main() {
    spdlog::info("Starting yw application...");

    // std::signal(SIGINT, handle_signal);
    // std::signal(SIGTERM, handle_signal);

    std::shared_ptr<yw::core::AppContext> app_context = std::make_shared<yw::core::AppContext>();
    if (!app_context->initialize()) {
        spdlog::error("Failed to initialize app context");
        return 1;
    }

    // 启动http服务
    app_context->runHttpServer();

    // 创建模块
    std::shared_ptr<yw::node::INodeModule> node_module = yw::node::NodeFactory::getNodeModule(app_context->getHttpService());
    std::shared_ptr<yw::monitor::IMonitorModule> monitor_module = yw::monitor::MonitorFactory::getMonitorModule(app_context->getHttpService(), node_module);
    std::shared_ptr<yw::bmc::IBMCModule> bmc_module = yw::bmc::BMCFactory::getBMCModule();
    
    // 创建告警模块（内部管理数据库连接）
    std::shared_ptr<yw::alert::IAlertModule> alert_module = yw::alert::AlertFactory::getAlertModule();
    
    std::shared_ptr<yw::web::IWebModule> web_module = yw::web::WebFactory::getWebModule(app_context->getHttpService(), node_module, monitor_module, bmc_module, alert_module);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    spdlog::info("Application finished.");

    return 0;
}