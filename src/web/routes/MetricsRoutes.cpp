#include "MetricsRoutes.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <spdlog/spdlog.h>
#include "dto/node_dto.h"
#include "mapper/NodeMapper.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerMetricsRoutes(hv::HttpService* service,
                           node::INodeModule* node_module,
                           monitor::IMonitorModule* monitor_module,
                           bmc::IBMCModule* bmc_module) {
    if (!service) return;

    // /node/metrics
    service->GET("/node/metrics", [node_module, monitor_module, bmc_module](const HttpContextPtr& ctx) {
        if (!node_module) {
            nlohmann::json resp = {{"api_version",1},{"status","error"},{"message","node module unavailable"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        
        const auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::system_clock::now()
        ).time_since_epoch().count();

        std::vector<NodeMetrics> result;
        int error_count = 0;
        
        try {
            auto nodes = node_module->getAllNodes();
            result.reserve(nodes.size());
            
            for (const auto& nx : nodes) {
                try {
                    // 获取监控资源（可选）
                    const monitor::Resource* res = nullptr;
                    std::optional<monitor::Resource> resHolder;
                    if (monitor_module) {
                        try {
                            auto resPtr = monitor_module->getNodeResource(nx.host_ip);
                            if (resPtr) { 
                                resHolder = *resPtr; 
                                res = &(*resHolder); 
                            }
                        } catch (const std::exception& e) {
                            spdlog::warn("Failed to get monitor resource for {}: {}", nx.host_ip, e.what());
                            // 继续处理，使用空的资源数据
                        }
                    }
                    
                    // 获取BMC传感器数据（可选）
                    std::unordered_map<std::string, bmc::BMCSensorRow> bmc_sensors;
                    if (bmc_module) {
                        try {
                            // 暂时不使用最新BMC传感器数据，因为数据量太大
                            // bmc_sensors = bmc_module->getLatestBMCSensor(nx.host_ip);
                        } catch (const std::exception& e) {
                            spdlog::warn("Failed to get BMC sensors for {}: {}", nx.host_ip, e.what());
                            // 继续处理，使用空的传感器数据
                        }
                    }
                    
                    result.push_back(yw::web::mapper::toNodeMetrics(nx, res, now_seconds, &bmc_sensors));
                    
                } catch (const std::exception& e) {
                    // 单个节点处理失败，记录日志但继续处理其他节点
                    error_count++;
                    spdlog::warn("Failed to process node {}: {}", nx.host_ip, e.what());
                    // 可选：添加一个带有错误标记的节点到结果中
                    // 或者跳过该节点（当前实现）
                }
            }
            
            // 如果所有节点都处理失败
            if (result.empty() && error_count > 0) {
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "All nodes processing failed"},
                    {"data", nlohmann::json::object()}
                };
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to get nodes list: {}", e.what());
            json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "Failed to retrieve nodes"},
                {"data", nlohmann::json::object()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }

        json resp = {
            {"api_version", 1},
            {"data", {
                {"nodes_metrics", result}
            }},
            {"status", "success"},
        };
        
        // 如果部分节点处理失败，在响应中增加警告信息（可选）
        if (error_count > 0) {
            resp["warnings"] = nlohmann::json::object();
            resp["warnings"]["failed_nodes_count"] = error_count;
        }
        
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // /node/historical-metrics - 查询节点历史指标数据
    service->GET("/node/historical-metrics", [node_module, monitor_module, bmc_module](const HttpContextPtr& ctx) {
        // ========== 1. 模块可用性检查 ==========
        if (!monitor_module) {
            json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "monitor module unavailable"},
                {"data", json::object()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }

        // ========== 2. 解析查询参数 ==========
        auto params = ctx->params();
        
        // 2a. 解析 host_ip（必需参数）
        std::string host_ip;
        if (params.find("host_ip") != params.end()) {
            host_ip = params["host_ip"];
            // 去除首尾空格
            if (!host_ip.empty()) {
                host_ip.erase(0, host_ip.find_first_not_of(" \t"));
                host_ip.erase(host_ip.find_last_not_of(" \t") + 1);
            }
        }
        
        if (host_ip.empty()) {
            json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "missing host_ip parameter"},
                {"data", json::object()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        
        // 2b. 解析 time_range（可选，默认 "1m"）
        std::string time_range = "1m";
        if (params.find("time_range") != params.end()) {
            time_range = params["time_range"];
            // 去除首尾空格
            if (!time_range.empty()) {
                time_range.erase(0, time_range.find_first_not_of(" \t"));
                time_range.erase(time_range.find_last_not_of(" \t") + 1);
            }
            if (time_range.empty()) {
                time_range = "1m";  // 如果去除空格后为空，恢复默认值
            }
        }
        
        // 2c. 解析 metrics（可选，逗号分隔的指标类型列表）
        std::vector<std::string> metrics_kinds;
        if (params.find("metrics") != params.end()) {
            const std::string& metrics_param = params["metrics"];
            if (!metrics_param.empty()) {
                std::string current_item;
                for (size_t i = 0; i <= metrics_param.size(); ++i) {
                    if (i == metrics_param.size() || metrics_param[i] == ',') {
                        // 去除首尾空格
                        if (!current_item.empty()) {
                            current_item.erase(0, current_item.find_first_not_of(" \t"));
                            current_item.erase(current_item.find_last_not_of(" \t") + 1);
                            if (!current_item.empty()) {
                                metrics_kinds.push_back(std::move(current_item));
                            }
                        }
                        current_item.clear();
                    } else {
                        current_item.push_back(metrics_param[i]);
                    }
                }
            }
        }

        // ========== 3. 查询监控指标序列数据 ==========
        monitor::MetricsSeries metrics_series;
        try {
            metrics_series = monitor_module->queryMetricsSeries(host_ip, time_range, metrics_kinds);
        } catch (const std::exception& e) {
            spdlog::error("Failed to query metrics series for {}: {}", host_ip, e.what());
            json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "Failed to query metrics series: " + std::string(e.what())},
                {"data", json::object()}
            };
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }

        // ========== 4. 构建 ResourceWindow ==========
        monitor::ResourceWindow win;
        win.host_ip = host_ip;
        win.time_range = time_range;
        win.metrics = std::move(metrics_series);
        
        // 4a. 填充节点信息（box_id, cpu_id, slot_id）
        if (node_module) {
            try {
                auto node_opt = node_module->getNodeByIP(host_ip);
                if (node_opt.has_value()) {
                    win.box_id = node_opt->box_id;
                    win.cpu_id = node_opt->cpu_id;
                    win.slot_id = node_opt->slot_id;
                }
            } catch (const std::exception& e) {
                spdlog::warn("Failed to get node info for {}: {}", host_ip, e.what());
                // 继续处理，使用默认值（0）
            }
        }

        // ========== 5. 获取BMC传感器数据（可选） ==========
        json* bmc_sensor_ptr = nullptr;
        json bmc_sensor_data;
        if (bmc_module) {
            try {
                auto bmc_sensor_map = bmc_module->queryBMCSensor(host_ip, time_range);
                bmc_sensor_data = bmc_sensor_map;  // 自动转换为JSON
                bmc_sensor_ptr = &bmc_sensor_data;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to get BMC sensors for {}: {}", host_ip, e.what());
                // 继续处理，不使用BMC传感器数据
            }
        }

        // ========== 6. 转换为视图对象 ==========
        auto historical_view = yw::web::mapper::toHistoricalMetricsView(win, bmc_sensor_ptr);

        // ========== 7. 构建并发送成功响应 ==========
        json resp = {
            {"api_version", 1},
            {"status", "success"},
            {"data", {
                {"historical_metrics", historical_view}
            }}
        };
        
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });
}

} // namespace routes
} // namespace web
} // namespace yw


