#include "yw/app_context.h"
#include <iostream>

namespace yw {
namespace core {

AppContext::~AppContext() {
    cleanup();
}

bool AppContext::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (http_server_) {
        std::cout << "AppContext already initialized" << std::endl;
        return true;
    }
    
    try {
        // 创建并启动libhv HttpServer实例
        http_server_ = std::make_unique<hv::HttpServer>();
        http_service_ = std::make_shared<hv::HttpService>();
        
        // 配置服务器（硬编码配置）
        http_server_->setHost("0.0.0.0");
        http_server_->setPort(8080);
        http_server_->setThreadNum(4);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize AppContext: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<hv::HttpService> AppContext::getHttpService() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_service_;
}

void AppContext::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (http_server_) {
        // 停止libhv服务器
        http_server_->stop();
        if (http_thread_.joinable()) {
            http_thread_.join();
        }
        
        // 重置HTTP服务器实例
        http_server_.reset();
        http_service_.reset();
        
        std::cout << "HTTP server stopped and AppContext cleaned up" << std::endl;
    }
}

void AppContext::runHttpServer() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!http_server_) {
        std::cerr << "HTTP server not initialized" << std::endl;
        return;
    }
    if (http_thread_.joinable()) {
        std::cout << "HTTP server thread already running" << std::endl;
        return;
    }
    http_thread_ = std::thread([srv = http_server_.get(), svc = http_service_]() {
        // 注册共享路由服务
        srv->registerHttpService(svc.get());
        int ret = srv->run();
        if (ret != 0) {
            std::cerr << "Failed to start HTTP server, error code: " << ret << std::endl;
            return;
        }
        std::cout << "HTTP server started on 0.0.0.0:8080" << std::endl;
    });
}

} // namespace core
} // namespace yw