// ============================================================================
// 文件功能描述：
// BMC路由（bmc_routes）的头文件，定义BMC和板卡控制相关HTTP API路由注册的接口。
// 主要功能包括：
// 1. 路由注册：提供registerBMCRoutes函数，注册BMC和板卡控制相关的HTTP API端点
// 2. BMC信息查询：GET /bmc - 查询所有机箱的BMC信息
// 3. 板卡控制：POST /bmc/{box_id}/reset、power-on、power-off - 控制板卡的复位和电源
// 4. 参数验证：验证槽位ID范围（1-12），验证请求参数格式
// ============================================================================

#pragma once

#include <hv/HttpService.h>

namespace yw {
namespace bmc {
    class IBMCModule;
}
namespace controller {
    class IControllerModule;
}

namespace web {
namespace routes {

/**
 * @brief 注册BMC相关的REST API路由
 * 
 * 包括BMC传感器查询、板卡电源控制等功能。
 * 
 * @param service HTTP服务
 * @param bmc_module BMC模块接口（可为空）
 * @param controller_module 控制器模块接口（可为空）
 */
void registerBMCRoutes(hv::HttpService* service,
                       bmc::IBMCModule* bmc_module,
                       controller::IControllerModule* controller_module);

} // namespace routes
} // namespace web
} // namespace yw


