#include "monitor/monitor.h"
#include "monitor_manager.h"
#include <mutex>
#include <hv/HttpService.h>

namespace yw {
namespace monitor {

std::shared_ptr<IMonitorModule> MonitorFactory::getMonitorModule(std::shared_ptr<hv::HttpService> service) {
    return std::make_shared<MonitorManager>(service);
}

} // namespace monitor
} // namespace yw

