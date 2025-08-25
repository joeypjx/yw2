#include "../../include/yw/web.h"
#include "web_controller.h"
#include <mutex>
#include <hv/HttpService.h>
#include "../../include/yw/node.h"
#include "../../include/yw/monitor.h"
#include "../../include/yw/bmc.h"

namespace yw {
namespace web {

std::shared_ptr<IWebModule> WebFactory::getWebModule(std::shared_ptr<hv::HttpService> service,
                                                     std::shared_ptr<node::INodeModule> node_module,
                                                     std::shared_ptr<monitor::IMonitorModule> monitor_module,
                                                     std::shared_ptr<bmc::IBMCModule> bmc_module,
                                                     std::shared_ptr<alert::IAlertModule> alert_module) {
    static std::shared_ptr<IWebModule> instance;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        instance = std::make_shared<WebController>(std::move(service), std::move(node_module), std::move(monitor_module), std::move(bmc_module), std::move(alert_module));
    }
    return instance;
}

} // namespace web
} // namespace yw


