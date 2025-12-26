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
    // 辅助函数：将节点转换为 NodeView JSON
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
    std::vector<std::string> normalizeExportTypes(const std::vector<std::string>& types) {
        if (types.empty()) {
            return {"cpu", "memory", "network", "disk", "gpu"};
        }
        bool has_system = std::find(types.begin(), types.end(), "system") != types.end();
        return has_system ? std::vector<std::string>{"cpu", "memory", "network", "disk", "gpu"} : types;
    }

    // 辅助函数：将 ExportDataPoint 转换为 JSON
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

void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module,
                        bmc::IBMCModule* bmc_module) {
                            
    if (!service) return;
    
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


