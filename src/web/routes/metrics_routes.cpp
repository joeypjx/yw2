#include "metrics_routes.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <spdlog/spdlog.h>
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "dto/node_metrics_dto.h"
#include "dto/historical_metrics_dto.h"
#include "mapper/node_mapper.h"
#include "utils/response_builder.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;
using ResponseBuilder = yw::utils::ResponseBuilder;

namespace {
    // 辅助函数：获取节点的监控资源
    const monitor::Resource* getNodeResource(const std::string& host_ip,
                                            monitor::IMonitorModule* monitor_module) {
        if (!monitor_module) {
            return nullptr;
        }
        try {
            auto resPtr = monitor_module->getNodeResource(host_ip);
            if (resPtr) {
                return resPtr.get();
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to get monitor resource for {}: {}", host_ip, e.what());
        }
        return nullptr;
    }

    // 辅助函数：获取节点的BMC传感器数据
    std::unordered_map<std::string, bmc::BMCSensorRow> getBMCSensors(const std::string& host_ip,
                                                                      bmc::IBMCModule* bmc_module) {
        if (!bmc_module) {
            return {};
        }
        try {
            return bmc_module->getLatestBMCSensor(host_ip);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to get BMC sensors for {}: {}", host_ip, e.what());
            return {};
        }
    }
}

void registerMetricsRoutes(hv::HttpService* service,
                           node::INodeModule* node_module,
                           monitor::IMonitorModule* monitor_module,
                           bmc::IBMCModule* bmc_module) {
    if (!service) return;

    // /node/metrics
    service->GET("/node/metrics", [node_module, monitor_module, bmc_module](const HttpContextPtr& ctx) {
        if (!node_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "node module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
        
        const auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::system_clock::now()
        ).time_since_epoch().count();

        std::vector<NodeMetrics> result;
        int error_count = 0;
        
        try {
            auto nodes = node_module->getAllNodes();
            result.reserve(nodes.size());
            
            for (const auto& node : nodes) {
                try {
                    // 获取监控资源（可选）
                    const monitor::Resource* res = getNodeResource(node.host_ip, monitor_module);
                    
                    // 获取BMC传感器数据（可选）
                    std::unordered_map<std::string, bmc::BMCSensorRow> bmc_sensors = getBMCSensors(node.host_ip, bmc_module);
                    
                    // 获取BMC prst（如果 bmc_module 可用）
                    std::optional<std::uint8_t> prst;
                    if (bmc_module) {
                        prst = bmc_module->getBoardPrst(node.box_id, node.slot_id);
                    }
                    
                    result.push_back(yw::web::mapper::toNodeMetrics(node, res, now_seconds, &bmc_sensors, prst));
                    
                } catch (const std::exception& e) {
                    // 单个节点处理失败，记录日志但继续处理其他节点
                    error_count++;
                    spdlog::warn("Failed to process node {}: {}", node.host_ip, e.what());
                }
            }
            
            // 如果所有节点都处理失败
            if (result.empty() && error_count > 0) {
                return ResponseBuilder::sendErrorWithReturn(ctx, "All nodes processing failed", HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to get nodes list: {}", e.what());
            return ResponseBuilder::sendErrorWithReturn(ctx, "Failed to retrieve nodes", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }

        json data = {
            {"nodes_metrics", result}
        };
        
        // 如果部分节点处理失败，在响应中增加警告信息（可选）
        if (error_count > 0) {
            data["warnings"] = json::object();
            data["warnings"]["failed_nodes_count"] = error_count;
        }
        
        return ResponseBuilder::sendSuccessWithReturn(ctx, data);
    });

    // /node/historical-metrics - 查询节点历史指标数据
    service->GET("/node/historical-metrics", [node_module, monitor_module, bmc_module](const HttpContextPtr& ctx) {
        // ========== 1. 模块可用性检查 ==========
        if (!monitor_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "monitor module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }

        // ========== 2. 解析查询参数 ==========
        auto params = ctx->params();
        
        // 2a. 解析 host_ip（必需参数）
        std::string host_ip = ResponseBuilder::getParam(params, "host_ip");
        if (host_ip.empty()) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "missing host_ip parameter", HTTP_STATUS_BAD_REQUEST);
        }
        
        // 2b. 解析 time_range（可选，默认 "1m"）
        std::string time_range = ResponseBuilder::getParam(params, "time_range", "1m");
        
        // 2c. 解析 metrics（可选，逗号分隔的指标类型列表）
        std::vector<std::string> metrics_kinds;
        std::string metrics_param = ResponseBuilder::getParam(params, "metrics");
        if (!metrics_param.empty()) {
            metrics_kinds = ResponseBuilder::parseCommaSeparated(metrics_param);
        }

        // ========== 3. 查询监控指标序列数据 ==========
        monitor::MetricsSeries metrics_series;
        try {
            metrics_series = monitor_module->queryMetricsSeries(host_ip, time_range, metrics_kinds);
        } catch (const std::exception& e) {
            spdlog::error("Failed to query metrics series for {}: {}", host_ip, e.what());
            return ResponseBuilder::sendErrorWithReturn(ctx, "Failed to query metrics series: " + std::string(e.what()), HTTP_STATUS_INTERNAL_SERVER_ERROR);
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
        json data = {
            {"historical_metrics", historical_view}
        };
        
        return ResponseBuilder::sendSuccessWithReturn(ctx, data);
    });
}

} // namespace routes
} // namespace web
} // namespace yw


