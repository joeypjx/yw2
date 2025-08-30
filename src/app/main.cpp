#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "yw/node.h"
#include "yw/monitor.h"
#include "yw/web.h"
#include "yw/bmc.h"
#include "yw/alert.h"
#include "yw/app_context.h"
#include "yw/JsonConfig.h"

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
    // 读取日志配置并初始化全局 logger
    // 先加载配置（允许日志初始化前读取默认值）
    (void)yw::utils::JsonConfig::Load("config/config.json");

    const std::string log_level = yw::utils::JsonConfig::Get<std::string>("logger.level", "info");
    const std::string log_pattern = yw::utils::JsonConfig::Get<std::string>("logger.pattern", "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    const bool log_console = yw::utils::JsonConfig::Get<bool>("logger.console", true);
    const std::string log_file = yw::utils::JsonConfig::Get<std::string>("logger.file", "logs/yw.log");
    const size_t log_max_size = static_cast<size_t>(yw::utils::JsonConfig::Get<int>("logger.max_size", 10 * 1024 * 1024));
    const size_t log_max_files = static_cast<size_t>(yw::utils::JsonConfig::Get<int>("logger.max_files", 5));

    std::vector<spdlog::sink_ptr> sinks;
    if (log_console) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
    if (!log_file.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file, log_max_size, log_max_files));
    }
    auto logger = std::make_shared<spdlog::logger>("yw", begin(sinks), end(sinks));
    spdlog::set_default_logger(logger);
    spdlog::set_pattern(log_pattern);
    auto to_level = [](const std::string& s){
        std::string t; t.reserve(s.size());
        for (char c : s) t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        if (t=="trace") return spdlog::level::trace;
        if (t=="debug") return spdlog::level::debug;
        if (t=="info")  return spdlog::level::info;
        if (t=="warn"||t=="warning")  return spdlog::level::warn;
        if (t=="error") return spdlog::level::err;
        if (t=="critical") return spdlog::level::critical;
        return spdlog::level::info;
    };
    spdlog::set_level(to_level(log_level));

    spdlog::info("Starting yw application...");

    // 加载全局配置
    // 再次加载配置（允许外部修改后覆盖前面默认加载）
    if (!yw::utils::JsonConfig::Load("config/config.json")) {
        spdlog::warn("JsonConfig load failed: config/config.json");
    } else {
        spdlog::info("JsonConfig loaded: config/config.json");
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

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
    
    // 注入到 AppContext 托管（共享实例持有）
    app_context->setNodeModule(node_module);
    app_context->setMonitorModule(monitor_module);
    app_context->setBMCModule(bmc_module);
    app_context->setAlertModule(alert_module);

    std::shared_ptr<yw::web::IWebModule> web_module = yw::web::WebFactory::getWebModule(app_context->getHttpServer(), app_context->getHttpService(), node_module, monitor_module, bmc_module, alert_module);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    app_context->cleanup();

    spdlog::info("Application finished.");

    return 0;
}