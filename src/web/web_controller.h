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


