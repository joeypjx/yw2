// ============================================================================
// 文件功能描述：
// 应用上下文（AppContext）的实现文件，负责管理整个应用程序的核心资源和生命周期。
// 主要功能包括：
// 1. HTTP服务器管理：创建和配置libhv HTTP服务器，设置监听地址、端口和线程数
// 2. HTTP服务管理：创建HttpService实例，用于注册API路由和处理HTTP请求
// 3. 模块生命周期管理：存储和管理所有业务模块（节点、监控、BMC、控制器、告警、Web）
// 4. 服务器启动：在独立线程中启动HTTP服务器，避免阻塞主线程
// 5. 资源清理：提供cleanup方法，按依赖顺序停止和销毁所有模块和服务器资源
// 6. 线程安全：使用互斥锁保护共享资源，确保多线程环境下的安全性
// ============================================================================

#include "core/app_context.h"
#include <spdlog/spdlog.h>
#include "utils/json_config.h"
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "web/web.h"

namespace yw {
namespace core {

// 应用上下文析构函数，自动清理资源
AppContext::~AppContext() {
    cleanup();
}

// 初始化应用上下文，创建HTTP服务器并配置
// 返回: 初始化成功返回true，失败返回false
bool AppContext::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (http_server_) {
        spdlog::info("AppContext already initialized");
        return true;
    }
    
    try {
        // 创建并启动libhv HttpServer实例
        http_server_ = std::make_shared<hv::HttpServer>();
        http_service_ = std::make_shared<hv::HttpService>();
        
        // 配置服务器（从配置加载）
        const std::string host = yw::utils::JsonConfig::Get<std::string>("host", "0.0.0.0");
        const int port = yw::utils::JsonConfig::Get<int>("port", 18888);
        const int threads = yw::utils::JsonConfig::Get<int>("thread_num", 4);
        http_server_->setHost(host.c_str());
        http_server_->setPort(port);
        http_server_->setThreadNum(threads);
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize AppContext: {}", e.what());
        return false;
    }
}

// 获取HTTP服务实例
// 返回: HTTP服务共享指针
std::shared_ptr<hv::HttpService> AppContext::getHttpService() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_service_;
}

// 获取HTTP服务器实例
// 返回: HTTP服务器共享指针
std::shared_ptr<hv::HttpServer> AppContext::getHttpServer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_server_;
}

// 清理应用上下文，停止所有模块和HTTP服务器
void AppContext::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 清理所有模块（按依赖顺序）
    // 注意：alert_module_ 的析构函数会自动调用 stop()，无需显式调用
    web_module_.reset();
    alert_module_.reset();
    node_module_.reset();
    monitor_module_.reset();
    bmc_module_.reset();
    controller_module_.reset();
    
    if (http_server_) {
        // 停止libhv服务器
        http_server_->stop();

        if (http_thread_.joinable()) {
            http_thread_.join();
        }
        
        // 重置HTTP服务器实例
        http_server_.reset();
        http_service_.reset();
        
        spdlog::info("HTTP server stopped and AppContext cleaned up");
    }
}

// 在独立线程中运行HTTP服务器
void AppContext::runHttpServer() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!http_server_) {
        spdlog::error("HTTP server not initialized");
        return;
    }
    if (http_thread_.joinable()) {
        spdlog::warn("HTTP server thread already running");
        return;
    }
    http_thread_ = std::thread([srv = http_server_, svc = http_service_]() {
        // 注册共享路由服务
        srv->registerHttpService(svc.get());
        int ret = srv->run();
        if (ret != 0) {
            spdlog::error("Failed to start HTTP server, error code: {}", ret);
            return;
        }
        spdlog::info("HTTP server started on {}:{}",
                     yw::utils::JsonConfig::Get<std::string>("host", "0.0.0.0"),
                     yw::utils::JsonConfig::Get<int>("port", 18888));
    });
}

// 设置节点模块
// m: 节点模块实例
void AppContext::setNodeModule(std::shared_ptr<yw::node::INodeModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_module_ = std::move(m);
}
// 设置监控模块
// m: 监控模块实例
void AppContext::setMonitorModule(std::shared_ptr<yw::monitor::IMonitorModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    monitor_module_ = std::move(m);
}
// 设置BMC模块
// m: BMC模块实例
void AppContext::setBMCModule(std::shared_ptr<yw::bmc::IBMCModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    bmc_module_ = std::move(m);
}
// 设置控制器模块
// m: 控制器模块实例
void AppContext::setControllerModule(std::shared_ptr<yw::controller::IControllerModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    controller_module_ = std::move(m);
}

// 获取节点模块
// 返回: 节点模块实例
std::shared_ptr<yw::node::INodeModule> AppContext::getNodeModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return node_module_;
}
// 获取监控模块
// 返回: 监控模块实例
std::shared_ptr<yw::monitor::IMonitorModule> AppContext::getMonitorModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return monitor_module_;
}
// 获取BMC模块
// 返回: BMC模块实例
std::shared_ptr<yw::bmc::IBMCModule> AppContext::getBMCModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bmc_module_;
}
// 获取控制器模块
// 返回: 控制器模块实例
std::shared_ptr<yw::controller::IControllerModule> AppContext::getControllerModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return controller_module_;
}

// 设置告警模块
// m: 告警模块实例
void AppContext::setAlertModule(std::shared_ptr<yw::alert::IAlertModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_module_ = std::move(m);
}

// 设置Web模块
// m: Web模块实例
void AppContext::setWebModule(std::shared_ptr<yw::web::IWebModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    web_module_ = std::move(m);
}

// 获取告警模块
// 返回: 告警模块实例
std::shared_ptr<yw::alert::IAlertModule> AppContext::getAlertModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return alert_module_;
}

// 获取Web模块
// 返回: Web模块实例
std::shared_ptr<yw::web::IWebModule> AppContext::getWebModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return web_module_;
}

} // namespace core
} // namespace yw