#pragma once

#include <hv/HttpService.h>
#include "yw/bmc.h"

namespace yw {
namespace web {
namespace routes {

void registerBMCRoutes(hv::HttpService* service,
                       bmc::IBMCModule* bmc_module);

} // namespace routes
} // namespace web
} // namespace yw


