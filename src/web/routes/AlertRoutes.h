#pragma once

#include <hv/HttpService.h>
#include "yw/alert.h"
#include "AlertPusher.h"

namespace yw {
namespace web {
namespace routes {

void registerAlertRoutes(hv::HttpService* service,
                         alert::IAlertModule* alert_module,
                         AlertPusher* pusher);

} // namespace routes
} // namespace web
} // namespace yw


