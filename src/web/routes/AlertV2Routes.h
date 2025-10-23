#pragma once

#include <hv/HttpService.h>
#include <memory>

namespace yw {
namespace alertv2 {
    class AlertEngine;
}

namespace web {
namespace routes {

/**
 * @brief 注册AlertV2的REST API路由
 * @param service HTTP服务
 * @param alertEngine AlertV2告警引擎
 */
void registerAlertV2Routes(hv::HttpService* service, 
                          std::shared_ptr<yw::alertv2::AlertEngine> alertEngine);

} // namespace routes
} // namespace web
} // namespace yw
