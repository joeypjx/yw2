// ============================================================================
// 文件功能描述：
// 节点路由（node_routes）的头文件，定义节点相关HTTP API路由注册的接口。
// 主要功能包括：
// 1. 路由注册：提供registerNodeRoutes函数，注册节点相关的HTTP API端点
// 2. 节点列表查询：GET /node - 查询所有节点信息，支持按类型、状态、机箱号等过滤
// 3. 节点详情查询：GET /node/{ip} - 查询指定节点的详细信息（包含监控资源和BMC状态）
// 4. 数据聚合：整合节点模块、监控模块和BMC模块的数据，提供统一的节点视图
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


