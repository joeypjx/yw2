// ============================================================================
// 文件功能描述：
// 应用上下文（AppContext）的头文件，定义应用程序核心资源管理的接口。
// 主要功能包括：
// 1. HTTP服务器管理：管理libhv HTTP服务器和HTTP服务的生命周期
// 2. 模块依赖注入：提供所有业务模块（节点、监控、BMC、控制器、告警、Web）的注入和获取接口
// 3. 线程安全：使用互斥锁保护共享资源，支持多线程并发访问
// 4. 资源清理：提供cleanup方法，按依赖顺序停止和销毁所有模块和服务器资源
// 5. 服务器启动：提供runHttpServer方法，在独立线程中启动HTTP服务器
// ============================================================================

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
namespace yw { namespace controller { class IControllerModule; } }
namespace yw { namespace alert { class IAlertModule; } }
namespace yw { namespace web { class IWebModule; } }
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
    std::shared_ptr<yw::controller::IControllerModule> controller_module_;
    std::shared_ptr<yw::alert::IAlertModule>     alert_module_;
    std::shared_ptr<yw::web::IWebModule>         web_module_;

public:
    // 注入/获取模块实例（线程安全，浅持有）
    void setNodeModule(std::shared_ptr<yw::node::INodeModule> m);
    void setMonitorModule(std::shared_ptr<yw::monitor::IMonitorModule> m);
    void setBMCModule(std::shared_ptr<yw::bmc::IBMCModule> m);
    void setControllerModule(std::shared_ptr<yw::controller::IControllerModule> m);
    void setAlertModule(std::shared_ptr<yw::alert::IAlertModule> m);
    void setWebModule(std::shared_ptr<yw::web::IWebModule> m);

    std::shared_ptr<yw::node::INodeModule> getNodeModule() const;
    std::shared_ptr<yw::monitor::IMonitorModule> getMonitorModule() const;
    std::shared_ptr<yw::bmc::IBMCModule> getBMCModule() const;
    std::shared_ptr<yw::controller::IControllerModule> getControllerModule() const;
    std::shared_ptr<yw::alert::IAlertModule> getAlertModule() const;
    std::shared_ptr<yw::web::IWebModule> getWebModule() const;
};

} // namespace core
} // namespace yw