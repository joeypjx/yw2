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

AppContext::~AppContext() {
    cleanup();
}

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

std::shared_ptr<hv::HttpService> AppContext::getHttpService() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_service_;
}

std::shared_ptr<hv::HttpServer> AppContext::getHttpServer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_server_;
}

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

// 模块注入/获取
void AppContext::setNodeModule(std::shared_ptr<yw::node::INodeModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_module_ = std::move(m);
}
void AppContext::setMonitorModule(std::shared_ptr<yw::monitor::IMonitorModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    monitor_module_ = std::move(m);
}
void AppContext::setBMCModule(std::shared_ptr<yw::bmc::IBMCModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    bmc_module_ = std::move(m);
}
void AppContext::setControllerModule(std::shared_ptr<yw::controller::IControllerModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    controller_module_ = std::move(m);
}

std::shared_ptr<yw::node::INodeModule> AppContext::getNodeModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return node_module_;
}
std::shared_ptr<yw::monitor::IMonitorModule> AppContext::getMonitorModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return monitor_module_;
}
std::shared_ptr<yw::bmc::IBMCModule> AppContext::getBMCModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bmc_module_;
}
std::shared_ptr<yw::controller::IControllerModule> AppContext::getControllerModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return controller_module_;
}

void AppContext::setAlertModule(std::shared_ptr<yw::alert::IAlertModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_module_ = std::move(m);
}

void AppContext::setWebModule(std::shared_ptr<yw::web::IWebModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    web_module_ = std::move(m);
}

std::shared_ptr<yw::alert::IAlertModule> AppContext::getAlertModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return alert_module_;
}

std::shared_ptr<yw::web::IWebModule> AppContext::getWebModule() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return web_module_;
}

} // namespace core
} // namespace yw