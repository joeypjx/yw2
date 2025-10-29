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
        if (!node_module) {
            json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "node module unavailable"},
                {"data", json::object()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }

        auto params = ctx->params();
        bool has_host_ip = params.find("host_ip") != params.end();
        bool has_box_id = params.find("box_id") != params.end();

        // 情况1: 有 host_ip 参数 - 返回单个节点对象（使用 INodeModule::getNodeByIP）
        if (has_host_ip) {
            const std::string filter_host_ip = params["host_ip"];
            auto nodeOpt = node_module->getNodeByIP(filter_host_ip);

            json resp;
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

                auto nodeView = yw::web::mapper::toNodeView(*nodeOpt, res);
                resp = {
                    {"api_version", 1},
                    {"data", nodeView},
                    {"status", "success"}
                };
            } else {
                resp = {
                    {"api_version", 1},
                    {"data", json::object()},
                    {"status", "success"}
                };
            }

            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        }

        // 情况2: 有 box_id 参数但无 host_ip - 返回指定 box_id 的节点列表
        if (has_box_id) {
            int filter_box_id;
            try {
                filter_box_id = std::stoi(params["box_id"]);
            } catch (...) {
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "invalid box_id"},
                    {"data", json::object()}
                };
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
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
                
                auto nodeView = yw::web::mapper::toNodeView(node, res);
                resp_nodes.push_back(std::move(nodeView));
            }

            json resp = {
                {"api_version", 1},
                {"data", {{"nodes", resp_nodes}}},
                {"status", "success"}
            };
            
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
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
            
            auto nodeView = yw::web::mapper::toNodeView(node, res);
            resp_nodes.push_back(std::move(nodeView));
        }

        json resp = {
            {"api_version", 1},
            {"data", {{"nodes", resp_nodes}}},
            {"status", "success"}
        };
        
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


