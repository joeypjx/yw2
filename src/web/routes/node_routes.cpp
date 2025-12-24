#include "node_routes.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>
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
                // 获取监控资源
                const monitor::Resource* res = nullptr;
                std::optional<monitor::Resource> resHolder;
                if (monitor_module) {
                    auto resPtr = monitor_module->getNodeResource(nodeOpt->host_ip);
                    if (resPtr) {
                        resHolder = *resPtr;
                        res = &(*resHolder);
                    }
                }

                auto prst = bmc_module->getBoardPrst(nodeOpt->box_id, nodeOpt->slot_id);
                auto nodeView = yw::web::mapper::toNodeView(*nodeOpt, res, prst);
                data = nodeView;
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
            
            for (const auto& node : nodes) {
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

                auto prst = bmc_module->getBoardPrst(node.box_id, node.slot_id);
                auto nodeView = yw::web::mapper::toNodeView(node, res, prst);
                resp_nodes.push_back(std::move(nodeView));
            }

            json data = {{"nodes", resp_nodes}};
            return ResponseBuilder::sendSuccessWithReturn(ctx, data);
        }

        // 情况3: 无参数 - 返回所有节点列表
        const auto nodes = node_module->getAllNodes();
        json resp_nodes = json::array();
        resp_nodes.get_ref<json::array_t&>().reserve(nodes.size());
        
        for (const auto& node : nodes) {
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
            
            auto prst = bmc_module->getBoardPrst(node.box_id, node.slot_id);
            auto nodeView = yw::web::mapper::toNodeView(node, res, prst);
            resp_nodes.push_back(std::move(nodeView));
        }

        json data = {{"nodes", resp_nodes}};
        return ResponseBuilder::sendSuccessWithReturn(ctx, data);
    });

    // GET /node/export - 导出节点历史数据
    service->GET("/node/export", [monitor_module](const HttpContextPtr& ctx) {
        auto t_start = std::chrono::high_resolution_clock::now();
        spdlog::info("[export] 接口开始处理请求");
        
        if (!monitor_module) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_INTERNAL_SERVER_ERROR, "monitor module unavailable");
        }

        auto t_params_start = std::chrono::high_resolution_clock::now();
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

        // 处理特殊类型值
        std::vector<std::string> actual_types;
        if (types.empty()) {
            // 如果未指定类型，默认包含所有指标
            actual_types = {"cpu", "memory", "network", "disk", "gpu"};
        } else {
            // 检查是否有特殊类型值
            bool has_system = std::find(types.begin(), types.end(), "system") != types.end();
            actual_types = has_system ? std::vector<std::string>{"cpu", "memory", "network", "disk", "gpu"} : types;
        }

        auto t_params_end = std::chrono::high_resolution_clock::now();
        auto params_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_params_end - t_params_start).count();
        spdlog::info("[export] 参数解析完成，耗时: {} ms", params_duration);

        try {
            auto t_export_start = std::chrono::high_resolution_clock::now();
            // 调用监控模块导出数据
            monitor::ExportData exportData = monitor_module->exportNodeData(ip_param, start_time, end_time, actual_types);
            auto t_export_end = std::chrono::high_resolution_clock::now();
            auto export_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_export_end - t_export_start).count();
            spdlog::info("[export] exportNodeData 调用完成，耗时: {} ms，数据点数量: {}", export_duration, exportData.data.size());

            auto t_json_start = std::chrono::high_resolution_clock::now();
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

                node_data["data"].push_back(point);
            }

            data_array.push_back(node_data);

            auto t_json_end = std::chrono::high_resolution_clock::now();
            auto json_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_json_end - t_json_start).count();
            spdlog::info("[export] JSON 转换完成，耗时: {} ms", json_duration);

            auto t_serialize_start = std::chrono::high_resolution_clock::now();
            json resp = {
                {"code", 200},
                {"data", data_array}
            };

            auto result = ResponseBuilder::sendJson(ctx, resp);
            auto t_serialize_end = std::chrono::high_resolution_clock::now();
            auto serialize_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_serialize_end - t_serialize_start).count();
            spdlog::info("[export] JSON 序列化完成，耗时: {} ms", serialize_duration);

            auto t_end = std::chrono::high_resolution_clock::now();
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
            spdlog::info("[export] 接口处理完成，总耗时: {} ms", total_duration);
            
            return result;

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorLegacyWithReturn(ctx, HTTP_STATUS_INTERNAL_SERVER_ERROR, "internal server error: " + std::string(e.what()));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw


