// ============================================================================
// 文件功能描述：
// Web模块（IWebModule）的头文件，定义Web服务模块的公共接口。
// 主要功能包括：
// 1. 路由管理接口：提供设置HTTP路由的接口
// 2. 工厂模式：提供WebFactory工厂类，用于创建Web模块实例
// 3. 依赖注入：接收HTTP服务器、HTTP服务和所有业务模块的引用
// 4. 接口抽象：定义IWebModule接口，隐藏具体实现细节，实现模块间的松耦合
// ============================================================================

#pragma once

#include <memory>

namespace hv { struct HttpService; class HttpServer; }

namespace yw {
namespace node { class INodeModule; }
namespace monitor { class IMonitorModule; }
namespace bmc { class IBMCModule; }
namespace alert { class IAlertModule; }
namespace controller { class IControllerModule; }
namespace web {

class IWebModule {
public:
    virtual ~IWebModule() = default;
    virtual void setupRoutes() = 0;
};

class WebFactory {
public:
    static std::shared_ptr<IWebModule> getWebModule(std::shared_ptr<hv::HttpServer> server,
                                                    std::shared_ptr<hv::HttpService> service,
                                                    std::shared_ptr<node::INodeModule> node_module,
                                                    std::shared_ptr<monitor::IMonitorModule> monitor_module,
                                                    std::shared_ptr<bmc::IBMCModule> bmc_module,
                                                    std::shared_ptr<controller::IControllerModule> controller_module,
                                                    std::shared_ptr<alert::IAlertModule> alert_module = nullptr);
};

} // namespace web
} // namespace yw


