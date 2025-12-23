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


