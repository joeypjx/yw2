#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <hv/hlog.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <csignal>
#include <chrono>
#include <thread>
#include <cctype>

#include "node/node.h"
#include "monitor/monitor.h"
#include "web/web.h"
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "core/app_context.h"
#include "utils/json_config.h"
#include "alert/alert.h"

namespace {

std::atomic<bool> g_running{true};

/**
 * @brief 信号处理函数
 */
void handle_signal(int) {
    g_running = false;
}

/**
 * @brief 将字符串转换为小写
 */
std::string toLower(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

/**
 * @brief 将日志级别字符串转换为 spdlog::level
 */
spdlog::level::level_enum parseLogLevel(const std::string& level_str) {
    const std::string level = toLower(level_str);
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "info") return spdlog::level::info;
    if (level == "warn" || level == "warning") return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    return spdlog::level::info;
}

/**
 * @brief 初始化日志系统
 * @param config_path 配置文件路径
 * @return 成功返回true，失败返回false
 */
bool initializeLogger(const std::string& config_path = "config/config.json") {
    // 先加载配置（允许日志初始化前读取默认值）
    if (!yw::utils::JsonConfig::Load(config_path)) {
        // 如果配置文件不存在，使用默认值继续
        spdlog::warn("Failed to load config file: {}, using defaults", config_path);
    }

    const std::string log_level = yw::utils::JsonConfig::Get<std::string>("logger.level", "info");
    const std::string log_pattern = yw::utils::JsonConfig::Get<std::string>("logger.pattern", 
        "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    const bool log_console = yw::utils::JsonConfig::Get<bool>("logger.console", true);
    const std::string log_file = yw::utils::JsonConfig::Get<std::string>("logger.file", "logs/yw.log");
    const size_t log_max_size = static_cast<size_t>(
        yw::utils::JsonConfig::Get<int>("logger.max_size", 10 * 1024 * 1024));
    const size_t log_max_files = static_cast<size_t>(
        yw::utils::JsonConfig::Get<int>("logger.max_files", 5));

    std::vector<spdlog::sink_ptr> sinks;
    if (log_console) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
    if (!log_file.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, log_max_size, log_max_files));
    }

    auto logger = std::make_shared<spdlog::logger>("yw", begin(sinks), end(sinks));
    spdlog::set_default_logger(logger);
    spdlog::set_pattern(log_pattern);
    spdlog::set_level(parseLogLevel(log_level));

    // 根据配置禁用libhv的日志输出
    const bool disable_libhv_log = yw::utils::JsonConfig::Get<bool>("logger.disable_libhv_log", true);
    if (disable_libhv_log) {
        hlog_disable();
        spdlog::info("libhv logging disabled");
    }

    return true;
}

/**
 * @brief 创建并启动告警模块
 * @param node_module 节点模块指针（用于回调）
 * @return IAlertModule 实例，失败返回 nullptr
 */
std::shared_ptr<yw::alert::IAlertModule> createAlertModule(
    std::shared_ptr<yw::node::INodeModule> node_module) {
    
    try {
        spdlog::info("Initializing alert module...");
        
        // 从配置文件读取数据库连接字符串
        const std::string conninfo = yw::utils::JsonConfig::Get<std::string>(
            "db.conninfo", 
            "postgres://postgres:HZ715Net@localhost:5432/yw"
        );
        
        // 使用工厂创建告警模块
        auto alert_module = yw::alert::AlertFactory::createAlertModule(
            conninfo, node_module.get());
        
        if (!alert_module) {
            spdlog::error("Failed to create alert module");
            return nullptr;
        }
        
        // 获取评估间隔（从配置读取，默认5秒）
        const int evaluation_interval = yw::utils::JsonConfig::Get<int>(
            "alert.evaluation_interval_seconds", 5);
        
        // 启动告警引擎
        alert_module->start(evaluation_interval);
        
        // 设置节点模块的告警回调
        node_module->setAlertCallback([alert_module](int box_id, int slot_id, 
                                                      const std::string& cached_board_type, 
                                                      const std::string& new_board_type) {
            if (alert_module) {
                alert_module->createBoardTypeChangeAlert(box_id, slot_id, cached_board_type, new_board_type);
            }
        });
        
        spdlog::info("Alert module initialized and started successfully");
        return alert_module;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize alert module: {}", e.what());
        spdlog::warn("Continuing without alert module");
        return nullptr;
    }
}

/**
 * @brief 创建所有业务模块并注入到 AppContext
 * @param app_context 应用程序上下文
 * @return 成功返回true，失败返回false
 */
bool createAndRegisterModules(std::shared_ptr<yw::core::AppContext> app_context) {
    // 创建核心模块
    auto node_module = yw::node::NodeFactory::getNodeModule(app_context->getHttpService());
    auto monitor_module = yw::monitor::MonitorFactory::getMonitorModule(
        app_context->getHttpService(), node_module);
    auto bmc_module = yw::bmc::BMCFactory::getBMCModule();
    auto controller_module = yw::controller::ControllerFactory::getControllerModule();
    
    // 创建告警模块（可选）
    auto alert_module = createAlertModule(node_module);
    
    // 注入到 AppContext 托管
    app_context->setNodeModule(node_module);
    app_context->setMonitorModule(monitor_module);
    app_context->setBMCModule(bmc_module);
    app_context->setControllerModule(controller_module);
    app_context->setAlertModule(alert_module);
    
    // 创建 Web 模块
    auto web_module = yw::web::WebFactory::getWebModule(
        app_context->getHttpServer(),
        app_context->getHttpService(),
        node_module,
        monitor_module,
        bmc_module,
        controller_module,
        alert_module
    );
    app_context->setWebModule(web_module);
    
    return true;
}

} // anonymous namespace

/**
 * @brief 应用程序入口点
 * 
 * 负责初始化日志系统、创建应用上下文、启动所有模块并运行主循环
 */
int main() {
    // 初始化日志系统（会自动加载配置）
    if (!initializeLogger("config/config.json")) {
        return 1;
    }

    spdlog::info("Starting yw application...");

    // 注册信号处理
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // 创建并初始化应用上下文
    auto app_context = std::make_shared<yw::core::AppContext>();
    if (!app_context->initialize()) {
        spdlog::error("Failed to initialize app context");
        return 1;
    }

    // 启动 HTTP 服务
    app_context->runHttpServer();

    // 创建并注册所有业务模块
    if (!createAndRegisterModules(app_context)) {
        spdlog::error("Failed to create modules");
        app_context->cleanup();
        return 1;
    }

    // 主循环
    spdlog::info("Application started successfully");
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 清理资源（会自动停止所有模块）
    spdlog::info("Shutting down application...");
    app_context->cleanup();
    spdlog::info("Application finished.");

    return 0;
}