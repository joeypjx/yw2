#pragma once

#include <hv/HttpService.h>
#include <memory>

namespace yw {
namespace alert {
    class IAlertModule;
}

namespace web {
namespace routes {

/**
 * @brief 注册告警相关的REST API路由
 * 
 * 包括告警规则管理、告警事件查询等功能。
 * 
 * @param service HTTP服务
 * @param alertModule 告警模块实例（智能指针）
 */
void registerAlertRoutes(hv::HttpService* service, 
                          std::shared_ptr<yw::alert::IAlertModule> alertModule);

} // namespace routes
} // namespace web
} // namespace yw
