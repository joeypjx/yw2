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


