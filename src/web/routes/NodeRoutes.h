#pragma once

#include <hv/HttpService.h>
#include "yw/node.h"
#include "yw/monitor.h"

namespace yw {
namespace web {
namespace routes {

void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module);

} // namespace routes
} // namespace web
} // namespace yw


