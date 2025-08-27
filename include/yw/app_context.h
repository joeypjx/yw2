#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <thread>

#include <hv/HttpServer.h>
#include <hv/HttpService.h>

namespace yw {
namespace core {

/**
 * @brief 应用程序上下文，管理HTTP服务器
 * 
 * 通过依赖注入的方式使用，提供HTTP服务器的生命周期管理
 */
class AppContext {
public:
    /**
     * @brief 构造函数
     */
    AppContext() = default;
    
    /**
     * @brief 析构函数
     */
    ~AppContext();

    // 禁止拷贝和赋值
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    // HTTP服务器管理
    /**
     * @brief 初始化AppContext，创建并启动HTTP服务器
     * @return 成功返回true，失败返回false
     */
    bool initialize();

    /**
     * @brief 获取共享的HttpService（用于直接注册路由）
     */
    std::shared_ptr<hv::HttpService> getHttpService() const;

    /**
     * @brief 获取底层 HttpServer 指针（用于注入给 AlertPusher）
     */
    hv::HttpServer* getHttpServer() const;

    /**
     * @brief 运行HTTP服务器
     */
    void runHttpServer();

    /**
     * @brief 清理资源，停止服务器并重置状态
     */
    void cleanup();

private:
    mutable std::mutex mutex_;                              // 全局锁
    
    // HTTP服务器相关
    std::unique_ptr<hv::HttpServer> http_server_;           // HTTP服务器（独占）
    std::shared_ptr<hv::HttpService> http_service_;         // 共享的路由服务
    std::thread                 http_thread_;               // HTTP服务器线程
};

} // namespace core
} // namespace yw