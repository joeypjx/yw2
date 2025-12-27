// ============================================================================
// 文件功能描述：
// Web控制器（WebController）的头文件，定义Web控制器的接口和实现。
// 主要功能包括：
// 1. HTTP路由管理：提供setupRoutes方法，注册所有业务模块的HTTP路由
// 2. 模块依赖管理：管理所有业务模块的引用（节点、监控、BMC、控制器、告警）
// 3. 告警推送：集成AlertPusher，实现告警事件的WebSocket实时推送
// 4. 生命周期管理：在析构时自动停止告警推送器，确保资源正确释放
// ============================================================================

#pragma once

#include <memory>
#include <hv/HttpService.h>
#include <hv/HttpServer.h>
#include "web/web.h"
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "services/alert_pusher.h"

// Forward declaration for AlertV2
namespace yw {
namespace alert {
    class IAlertModule;
}
}

namespace yw {
namespace web {

// Web控制器，负责管理HTTP路由和WebSocket推送
class WebController : public IWebModule {
public:
    // 构造函数，初始化所有模块依赖
    // server: HTTP服务器实例
    // service: HTTP服务实例
    // node_module: 节点模块
    // monitor_module: 监控模块
    // bmc_module: BMC模块
    // controller_module: 控制器模块
    // alert_module: 告警模块（可选）
    WebController(std::shared_ptr<hv::HttpServer> server,
                  std::shared_ptr<hv::HttpService> service,
                  std::shared_ptr<node::INodeModule> node_module,
                  std::shared_ptr<monitor::IMonitorModule> monitor_module,
                  std::shared_ptr<bmc::IBMCModule> bmc_module,
                  std::shared_ptr<controller::IControllerModule> controller_module,
                  std::shared_ptr<alert::IAlertModule> alert_module = nullptr);

    // 析构函数
    ~WebController() override;

    // 设置HTTP路由，注册所有API端点
    void setupRoutes() override;

    

private:
    std::shared_ptr<hv::HttpServer>         server_;
    std::shared_ptr<hv::HttpService>        service_;
    std::shared_ptr<node::INodeModule>      node_module_;
    std::shared_ptr<monitor::IMonitorModule> monitor_module_;
    std::shared_ptr<bmc::IBMCModule>        bmc_module_;
    std::shared_ptr<controller::IControllerModule> controller_module_;
    std::shared_ptr<alert::IAlertModule> alert_module_;

    std::unique_ptr<AlertPusher>            pusher_;
};

} // namespace web
} // namespace yw


