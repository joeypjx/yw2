#pragma once

#include <hv/HttpService.h>
#include "yw/node.h"
#include "yw/monitor.h"
#include "yw/bmc.h"

namespace yw {
namespace web {
namespace routes {

void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module,
                        bmc::IBMCModule* bmc_module);

} // namespace routes
} // namespace web
} // namespace yw


