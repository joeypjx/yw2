#include "../../include/yw/monitor.h"
#include "monitor_manager.h"
#include <mutex>
#include <hv/HttpService.h>
#include "../node/node_manager.h"

namespace yw {
namespace monitor {

std::shared_ptr<IMonitorModule> MonitorFactory::getMonitorModule(std::shared_ptr<hv::HttpService> service,
                                                                 std::shared_ptr<node::INodeModule> node_module) {
    static std::shared_ptr<IMonitorModule> instance;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        instance = std::make_shared<MonitorManager>(service, std::move(node_module));
    }
    return instance;
}

} // namespace monitor
} // namespace yw

