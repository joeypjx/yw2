// ============================================================================
// 文件功能描述：
// 指标路由（metrics_routes）的头文件，定义节点监控指标相关HTTP API路由注册的接口。
// 主要功能包括：
// 1. 路由注册：提供registerMetricsRoutes函数，注册节点监控指标相关的HTTP API端点
// 2. 最新指标查询：GET /node/{ip}/metrics - 查询指定节点的最新监控指标
// 3. 历史指标查询：GET /node/{ip}/historical-metrics - 查询指定节点在指定时间范围内的历史监控指标时序数据
// 4. 数据聚合：整合监控模块和BMC模块的数据，提供完整的节点指标视图
// ============================================================================

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
 * @brief 注册节点指标相关的REST API路由
 * 
 * 包括节点实时指标查询、历史指标查询等功能。
 * 
 * @param service HTTP服务
 * @param node_module 节点模块接口（可为空）
 * @param monitor_module 监控模块接口（可为空）
 * @param bmc_module BMC模块接口（可为空）
 */
void registerMetricsRoutes(hv::HttpService* service,
                           node::INodeModule* node_module,
                           monitor::IMonitorModule* monitor_module,
                           bmc::IBMCModule* bmc_module);

} // namespace routes
} // namespace web
} // namespace yw


