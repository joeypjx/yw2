// ============================================================================
// 文件功能描述：
// Web工厂（WebFactory）的实现文件，负责创建和初始化Web模块实例。
// 主要功能包括：
// 1. 模块创建：创建WebController实例，统一管理所有HTTP路由和Web服务
// 2. 依赖注入：接收HTTP服务器、HTTP服务和所有业务模块的引用，实现模块间的松耦合
// 3. 工厂模式：提供统一的模块创建接口，隐藏具体的实现细节
// ============================================================================

#include "web/web.h"
#include "web_controller.h"
#include <mutex>
#include <hv/HttpService.h>
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "alert/alert.h"

namespace yw {
namespace web {

// 创建Web模块实例
// server: HTTP服务器实例
// service: HTTP服务实例
// node_module: 节点模块
// monitor_module: 监控模块
// bmc_module: BMC模块
// controller_module: 控制器模块
// alert_module: 告警模块
// 返回: Web模块共享指针
std::shared_ptr<IWebModule> WebFactory::getWebModule(std::shared_ptr<hv::HttpServer> server,
                                                     std::shared_ptr<hv::HttpService> service,
                                                     std::shared_ptr<node::INodeModule> node_module,
                                                     std::shared_ptr<monitor::IMonitorModule> monitor_module,
                                                     std::shared_ptr<bmc::IBMCModule> bmc_module,
                                                     std::shared_ptr<controller::IControllerModule> controller_module,
                                                     std::shared_ptr<alert::IAlertModule> alert_module) {
    return std::make_shared<WebController>(
        std::move(server), 
        std::move(service), 
        std::move(node_module), 
        std::move(monitor_module), 
        std::move(bmc_module), 
        std::move(controller_module), 
        std::move(alert_module)
    );
}

} // namespace web
} // namespace yw


