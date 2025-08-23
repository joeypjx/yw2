#pragma once

#include <memory>
#include <string>
#include <mutex>

// 前向声明
namespace hv {
    class HttpServer;
}

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
    ~AppContext() = default;

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
     * @brief 获取HTTP服务器实例
     * @return HTTP服务器智能指针，未初始化则返回nullptr
     */
    std::shared_ptr<hv::HttpServer> getHttpServer() const;

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
    std::shared_ptr<hv::HttpServer> http_server_;           // HTTP服务器实例
};

} // namespace core
} // namespace yw