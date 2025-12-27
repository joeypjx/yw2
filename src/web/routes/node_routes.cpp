// ============================================================================
// 文件功能描述：
// 节点路由（node_routes）的实现文件，提供节点相关的HTTP API端点。
// 主要功能包括：
// 1. 节点列表查询：GET /node - 查询所有节点信息，支持按类型、状态、机箱号等过滤
// 2. 节点详情查询：GET /node/{ip} - 查询指定节点的详细信息（包含监控资源和BMC状态）
// 3. 节点视图转换：将节点扩展信息转换为NodeView对象，包含节点基本信息和组件列表
// 4. 数据聚合：整合节点模块、监控模块和BMC模块的数据，提供统一的节点视图
// 5. 状态判断：根据节点在线状态和板卡在位状态（PRST）判断节点状态
// 6. 参数解析：解析HTTP请求参数（类型、状态、机箱号等），构建查询条件
// ============================================================================

#include "node_routes.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "dto/node_view_dto.h"
#include "mapper/node_mapper.h"
#include "utils/response_builder.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;
using ResponseBuilder = yw::utils::ResponseBuilder;

namespace {
    // 辅助函数：将节点扩展信息转换为 NodeView JSON
    // node: 节点扩展信息（包含节点数据和更新时间）
    // monitor_module: 监控模块实例（可为nullptr）
    // bmc_module: BMC模块实例（可为nullptr）
    // 返回: NodeView JSON对象
    json convertNodeToView(const node::NodeExt& node,
                          monitor::IMonitorModule* monitor_module,
                          bmc::IBMCModule* bmc_module) {
        // 获取监控资源
        const monitor::Resource* res = nullptr;
        std::optional<monitor::Resource> resHolder;
        if (monitor_module) {
            auto resPtr = monitor_module->getNodeResource(node.host_ip);
            if (resPtr) {
                resHolder = *resPtr;
                res = &(*resHolder);
            }
        }

        // 获取 BMC prst（如果 bmc_module 可用）
        std::optional<std::uint8_t> prst;
        if (bmc_module) {
            prst = bmc_module->getBoardPrst(node.box_id, node.slot_id);
        }

        return yw::web::mapper::toNodeView(node, res, prst);
    }

    // 辅助函数：处理类型参数，将 "system" 展开为所有类型
    // types: 指标类型列表（如["cpu", "memory"]）
    // 返回: 标准化后的类型列表（如果包含"system"则展开为所有类型，空列表则返回默认类型）
    std::vector<std::string> normalizeExportTypes(const std::vector<std::string>& types) {
        if (types.empty()) {
            return {"cpu", "memory", "network", "disk", "gpu"};
        }
        bool has_system = std::find(types.begin(), types.end(), "system") != types.end();
        return has_system ? std::vector<std::string>{"cpu", "memory", "network", "disk", "gpu"} : types;
    }

    // 辅助函数：将导出数据点转换为 JSON
    // dataPoint: 导出数据点（包含时间戳和各种指标值）
    // 返回: JSON对象（包含时间戳、CPU使用率、内存使用率、磁盘使用率、网络速率、GPU使用率等）
    json convertDataPointToJson(const monitor::ExportDataPoint& dataPoint) {
        json point = {
            {"timestamp", dataPoint.timestamp},
            {"cpu_usage_percent", dataPoint.cpu_usage_percent},
            {"memory_usage_percent", dataPoint.memory_usage_percent}
        };

        // 添加磁盘使用率
        for (const auto& [mount_point, usage] : dataPoint.disk_usage_percent) {
            point["disk_" + mount_point + "_usage_percent"] = usage;
        }

        // 添加网络速率
        for (const auto& [interface, rate] : dataPoint.network_rx_rate) {
            point["network_" + interface + "_rx_rate"] = static_cast<std::int64_t>(rate);
        }
        for (const auto& [interface, rate] : dataPoint.network_tx_rate) {
            point["network_" + interface + "_tx_rate"] = static_cast<std::int64_t>(rate);
        }

        // 添加GPU使用率
        for (const auto& [gpu_index, usage] : dataPoint.gpu_compute_usage) {
            point["gpu_" + gpu_index + "_compute_usage"] = usage;
        }
        for (const auto& [gpu_index, usage] : dataPoint.gpu_mem_usage) {
            point["gpu_" + gpu_index + "_mem_usage"] = usage;
        }

        return point;
    }
}

// 注册节点相关的HTTP路由
// service: HTTP服务实例
// node_module: 节点模块实例
// monitor_module: 监控模块实例
// bmc_module: BMC模块实例
void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module,
                        bmc::IBMCModule* bmc_module) {
                            
    if (!service) return;
    
    // GET /node - 获取节点列表或单个节点
    // 查询参数：host_ip（可选）- 节点IP地址
    // 查询参数：box_id（可选）- 机箱编号
    // 返回：节点列表或单个节点JSON对象
    service->GET("/node", [node_module, monitor_module, bmc_module](const HttpContextPtr& ctx) {
        if (!node_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "node module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }

        auto params = ctx->params();
        bool has_host_ip = ResponseBuilder::hasParam(params, "host_ip");
        bool has_box_id = ResponseBuilder::hasParam(params, "box_id");

        // 情况1: 有 host_ip 参数 - 返回单个节点对象（使用 INodeModule::getNodeByIP）
        if (has_host_ip) {
            const std::string filter_host_ip = ResponseBuilder::getParam(params, "host_ip");
            auto nodeOpt = node_module->getNodeByIP(filter_host_ip);

            json data;
            if (nodeOpt.has_value()) {
                data = convertNodeToView(*nodeOpt, monitor_module, bmc_module);
            } else {
                data = json::object();
            }

            return ResponseBuilder::sendSuccessWithReturn(ctx, data);
        }

        // 情况2: 有 box_id 参数但无 host_ip - 返回指定 box_id 的节点列表
        if (has_box_id) {
            int filter_box_id = ResponseBuilder::getIntParam(params, "box_id", -1);
            if (filter_box_id < 0) {
                return ResponseBuilder::sendErrorWithReturn(ctx, "invalid box_id", HTTP_STATUS_BAD_REQUEST);
            }

            const auto nodes = node_module->getNodesByBoxId(filter_box_id);
            json resp_nodes = json::array();
            resp_nodes.get_ref<json::array_t&>().reserve(nodes.size());
            
            for (const auto& node : nodes) {
                resp_nodes.push_back(convertNodeToView(node, monitor_module, bmc_module));
            }

            json data = {{"nodes", resp_nodes}};
            return ResponseBuilder::sendSuccessWithReturn(ctx, data);
        }

        // 情况3: 无参数 - 返回所有节点列表
        const auto nodes = node_module->getAllNodes();
        json resp_nodes = json::array();
        resp_nodes.get_ref<json::array_t&>().reserve(nodes.size());
        
        for (const auto& node : nodes) {
            resp_nodes.push_back(convertNodeToView(node, monitor_module, bmc_module));
        }

        json data = {{"nodes", resp_nodes}};
        return ResponseBuilder::sendSuccessWithReturn(ctx, data);
    });

    // GET /node/export - 导出节点历史数据
    service->GET("/node/export", [monitor_module](const HttpContextPtr& ctx) {
        if (!monitor_module) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_INTERNAL_SERVER_ERROR, "monitor module unavailable");
        }

        auto params = ctx->params();
        
        // 解析必需参数
        std::string ip_param = ResponseBuilder::getParam(params, "ip");
        std::string start_time_param = ResponseBuilder::getParam(params, "start_time");
        std::string end_time_param = ResponseBuilder::getParam(params, "end_time");
        
        if (ip_param.empty() || start_time_param.empty() || end_time_param.empty()) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_BAD_REQUEST, "missing required parameters: ip, start_time, end_time");
        }

        // 解析时间戳
        std::int64_t start_time, end_time;
        try {
            start_time = std::stoll(start_time_param);
            end_time = std::stoll(end_time_param);
        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_BAD_REQUEST, "invalid timestamp format");
        }

        // 验证时间范围
        if (start_time >= end_time) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_BAD_REQUEST, "start_time must be less than end_time");
        }

        // 解析可选的类型参数
        std::string type_param = ResponseBuilder::getParam(params, "type");
        std::vector<std::string> types = type_param.empty() ? std::vector<std::string>{} : ResponseBuilder::parseCommaSeparated(type_param);
        std::vector<std::string> actual_types = normalizeExportTypes(types);

        try {
            // 调用监控模块导出数据
            monitor::ExportData exportData = monitor_module->exportNodeData(ip_param, start_time, end_time, actual_types);

            // 转换为 export.md 格式
            json data_array = json::array();
            
            json node_data = {
                {"start_time", exportData.start_time},
                {"end_time", exportData.end_time},
                {"ip", exportData.ip},
                {"type", exportData.type},
                {"data", json::array()}
            };

            // 转换数据点
            for (const auto& dataPoint : exportData.data) {
                node_data["data"].push_back(convertDataPointToJson(dataPoint));
            }

            data_array.push_back(node_data);

            json resp = {
                {"code", 200},
                {"data", data_array}
            };

            return ResponseBuilder::sendJson(ctx, resp);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_INTERNAL_SERVER_ERROR, "internal server error: " + std::string(e.what()));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw


