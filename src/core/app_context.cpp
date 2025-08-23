#include "yw/app_context.h"
#include <iostream>
#include <hv/HttpServer.h>

namespace yw {
namespace core {

bool AppContext::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (http_server_) {
        std::cout << "AppContext already initialized" << std::endl;
        return true;
    }
    
    try {
        // 创建并启动libhv HttpServer实例
        http_server_ = std::make_shared<hv::HttpServer>();
        
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

std::shared_ptr<hv::HttpServer> AppContext::getHttpServer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_server_;
}

void AppContext::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (http_server_) {
        // 停止libhv服务器
        http_server_->stop();
        
        // 重置HTTP服务器实例
        http_server_.reset();
        
        std::cout << "HTTP server stopped and AppContext cleaned up" << std::endl;
    }
}

void AppContext::runHttpServer() {
    // 启动服务器
    int ret = http_server_->run();
    if (ret != 0) {
        std::cerr << "Failed to start HTTP server, error code: " << ret << std::endl;
        return;
    }

    std::cout << "HTTP server started on 0.0.0.0:8080" << std::endl;
}

} // namespace core
} // namespace yw