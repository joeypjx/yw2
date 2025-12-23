#pragma once

#include <hv/HttpService.h>

namespace yw {
namespace node {
    class INodeModule;
}
namespace monitor {
    class IMonitorModule;
}
namespace bmc {
    class IBMCModule;
}

namespace web {
namespace routes {

/**
 * @brief 注册节点相关的REST API路由
 * 
 * 包括节点列表查询、节点详情查询、节点历史数据导出等功能。
 * 
 * @param service HTTP服务
 * @param node_module 节点模块接口（可为空）
 * @param monitor_module 监控模块接口（可为空）
 * @param bmc_module BMC模块接口（可为空）
 */
void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module,
                        bmc::IBMCModule* bmc_module);

} // namespace routes
} // namespace web
} // namespace yw


