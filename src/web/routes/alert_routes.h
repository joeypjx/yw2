// ============================================================================
// 文件功能描述：
// 告警路由（alert_routes）的头文件，定义告警规则和告警事件相关HTTP API路由注册的接口。
// 主要功能包括：
// 1. 路由注册：提供registerAlertRoutes函数，注册告警相关的HTTP API端点
// 2. 告警规则管理：POST/GET/PUT/DELETE /alert/rules - 告警规则的增删改查
// 3. 告警查询：GET /alert/alerts - 查询告警事件，支持按状态、严重程度、类型、IP、机箱号等过滤
// 4. 告警详情：GET /alert/alerts/{id} - 查询指定告警事件的详细信息
// 5. 告警统计：GET /alert/count - 获取告警总数
// ============================================================================

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
