#pragma once

#include <hv/HttpService.h>
#include <memory>

namespace yw {
namespace alert {
    class AlertEngine;
}

namespace web {
namespace routes {

/**
 * @brief 注册Alert的REST API路由
 * @param service HTTP服务
 * @param alertEngine Alert告警引擎
 */
void registerAlertRoutes(hv::HttpService* service, 
                          std::shared_ptr<yw::alert::AlertEngine> alertEngine);

} // namespace routes
} // namespace web
} // namespace yw
