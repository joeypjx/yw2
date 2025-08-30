#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <thread>

#include <hv/HttpServer.h>
#include <hv/HttpService.h>

// 前置声明业务接口
namespace yw { namespace node { class INodeModule; } }
namespace yw { namespace monitor { class IMonitorModule; } }
namespace yw { namespace bmc { class IBMCModule; } }
namespace yw { namespace alert { class IAlertModule; } }

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
     * @brief 获取底层 HttpServer 指针
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
    std::shared_ptr<hv::HttpServer> http_server_;           // HTTP服务器
    std::shared_ptr<hv::HttpService> http_service_;         // 共享的路由服务
    std::thread                 http_thread_;               // HTTP服务器线程

    // 共享模块实例（由 AppContext 托管生命周期，可选持有）
    std::shared_ptr<yw::node::INodeModule>       node_module_;
    std::shared_ptr<yw::monitor::IMonitorModule> monitor_module_;
    std::shared_ptr<yw::bmc::IBMCModule>         bmc_module_;
    std::shared_ptr<yw::alert::IAlertModule>     alert_module_;

public:
    // 注入/获取模块实例（线程安全，浅持有）
    void setNodeModule(std::shared_ptr<yw::node::INodeModule> m);
    void setMonitorModule(std::shared_ptr<yw::monitor::IMonitorModule> m);
    void setBMCModule(std::shared_ptr<yw::bmc::IBMCModule> m);
    void setAlertModule(std::shared_ptr<yw::alert::IAlertModule> m);

    std::shared_ptr<yw::node::INodeModule> getNodeModule() const;
    std::shared_ptr<yw::monitor::IMonitorModule> getMonitorModule() const;
    std::shared_ptr<yw::bmc::IBMCModule> getBMCModule() const;
    std::shared_ptr<yw::alert::IAlertModule> getAlertModule() const;
};

} // namespace core
} // namespace yw