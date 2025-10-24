#include "NodeRoutes.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include "dto/node_dto.h"
#include "mapper/NodeMapper.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module) {
    if (!service) return;
    service->GET("/node", [node_module, monitor_module](const HttpContextPtr& ctx) {
        if (!node_module) return 500;
        const auto nodes = node_module->getAllNodes();
        int filter_box_id = -1;
        bool has_filter = false;
        std::string filter_host_ip;
        bool has_host_filter = false;
        auto params = ctx->params();
        if (params.find("box_id") != params.end()) {
            try {
                filter_box_id = std::stoi(params["box_id"]);
                has_filter = true;
            } catch (...) {
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "invalid box_id"},
                    {"data", nlohmann::json::object()}
                };
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
        }
        if (params.find("host_ip") != params.end()) {
            filter_host_ip = params["host_ip"];
            has_host_filter = true;
        }
        nlohmann::json resp_nodes = nlohmann::json::array();
        resp_nodes.get_ref<nlohmann::json::array_t&>().reserve(nodes.size());
        for (const auto& ext : nodes) {
            if (has_filter && ext.box_id != filter_box_id) continue;
            if (has_host_filter && ext.host_ip != filter_host_ip) continue;
            const monitor::Resource* res = nullptr;
            std::optional<monitor::Resource> resHolder;
            if (monitor_module) {
                auto resPtr = monitor_module->getNodeResource(ext.host_ip);
                if (resPtr) { resHolder = *resPtr; res = &(*resHolder); }
            }
            auto v = yw::web::mapper::toNodeView(ext, res);
            resp_nodes.push_back(std::move(v));
        }

        nlohmann::json resp;
        if (has_host_filter) {
            if (!resp_nodes.empty()) {
                resp = {
                    {"api_version", 1},
                    {"data", resp_nodes[0]},
                    {"status", "success"},
                };
            } else {
                resp = {
                    {"api_version", 1},
                    {"data", json::object()},
                    {"status", "success"},
                };
            }
        } else {
            resp = {
                {"api_version", 1},
                {"data", {
                    {"nodes", resp_nodes}
                }},
                {"status", "success"},
            };
        }

        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // GET /node/export - 导出节点历史数据
    service->GET("/node/export", [monitor_module](const HttpContextPtr& ctx) {
        if (!monitor_module) {
            nlohmann::json resp = {
                {"code", 400},
                {"message", "monitor module unavailable"},
                {"data", nlohmann::json::array()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }

        auto params = ctx->params();
        
        // 解析必需参数
        std::string ip_param = params["ip"];
        std::string start_time_param = params["start_time"];
        std::string end_time_param = params["end_time"];
        
        if (ip_param.empty() || start_time_param.empty() || end_time_param.empty()) {
            nlohmann::json resp = {
                {"code", 400},
                {"message", "missing required parameters: ip, start_time, end_time"},
                {"data", nlohmann::json::array()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }

        // 解析时间戳
        std::int64_t start_time, end_time;
        try {
            start_time = std::stoll(start_time_param);
            end_time = std::stoll(end_time_param);
        } catch (const std::exception& e) {
            nlohmann::json resp = {
                {"code", 400},
                {"message", "invalid timestamp format"},
                {"data", nlohmann::json::array()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }

        // 验证时间范围
        if (start_time >= end_time) {
            nlohmann::json resp = {
                {"code", 400},
                {"message", "start_time must be less than end_time"},
                {"data", nlohmann::json::array()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }

        // 解析可选的类型参数
        std::vector<std::string> types;
        std::string type_param = params["type"];
        if (!type_param.empty()) {
            // 支持逗号分隔的多个类型
            std::stringstream ss(type_param);
            std::string type;
            while (std::getline(ss, type, ',')) {
                // 去除空格
                type.erase(0, type.find_first_not_of(" \t"));
                type.erase(type.find_last_not_of(" \t") + 1);
                if (!type.empty()) {
                    types.push_back(type);
                }
            }
        }

        // 处理特殊类型值
        std::vector<std::string> actual_types;
        if (types.empty()) {
            // 如果未指定类型，默认包含所有指标
            actual_types = {"cpu", "memory", "network", "disk", "gpu"};
        } else {
            // 检查是否有特殊类型值
            bool has_system = false;
            for (const auto& type : types) {
                if (type == "system") {
                    has_system = true;
                    break;
                }
            }
            
            if (has_system) {
                // 如果包含 "system"，则包含所有指标
                actual_types = {"cpu", "memory", "network", "disk", "gpu"};
            } else {
                // 否则使用指定的类型（为将来扩展做准备）
                actual_types = types;
            }
        }

        try {
            // 调用监控模块导出数据
            monitor::ExportData exportData = monitor_module->exportNodeData(ip_param, start_time, end_time, actual_types);

            // 转换为 export.md 格式
            nlohmann::json data_array = nlohmann::json::array();
            
            nlohmann::json node_data = {
                {"start_time", exportData.start_time},
                {"end_time", exportData.end_time},
                {"ip", exportData.ip},
                {"type", exportData.type},
                {"data", nlohmann::json::array()}
            };

            // 转换数据点
            for (const auto& dataPoint : exportData.data) {
                nlohmann::json point = {
                    {"timestamp", dataPoint.timestamp},
                    {"cpu_usage_percent", dataPoint.cpu_usage_percent},
                    {"memory_usage_percent", dataPoint.memory_usage_percent}
                };

                // 添加磁盘使用率
                for (const auto& [mount_point, usage] : dataPoint.disk_usage_percent) {
                    std::string key = "disk_" + mount_point + "_usage_percent";
                    point[key] = usage;
                }

                // 添加网络速率
                for (const auto& [interface, rate] : dataPoint.network_rx_rate) {
                    std::string key = "network_" + interface + "_rx_rate";
                    point[key] = static_cast<std::int64_t>(rate);
                }
                for (const auto& [interface, rate] : dataPoint.network_tx_rate) {
                    std::string key = "network_" + interface + "_tx_rate";
                    point[key] = static_cast<std::int64_t>(rate);
                }

                // 添加GPU使用率
                for (const auto& [gpu_index, usage] : dataPoint.gpu_compute_usage) {
                    std::string key = "gpu_" + gpu_index + "_compute_usage";
                    point[key] = usage;
                }
                for (const auto& [gpu_index, usage] : dataPoint.gpu_mem_usage) {
                    std::string key = "gpu_" + gpu_index + "_mem_usage";
                    point[key] = usage;
                }

                node_data["data"].push_back(point);
            }

            data_array.push_back(node_data);

            nlohmann::json resp = {
                {"code", 200},
                {"data", data_array}
            };

            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            nlohmann::json resp = {
                {"code", 500},
                {"message", "internal server error: " + std::string(e.what())},
                {"data", nlohmann::json::array()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw


