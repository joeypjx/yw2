#pragma once

#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "yw/node_model.h"
#include "yw/monitor_model.h"
#include "yw/bmc_model.h"
#include "dto/node_dto.h"

namespace yw {
namespace web {
namespace mapper {

// NodeExt + 资源（可为空）-> NodeView
NodeView toNodeView(const node::NodeExt& ext, const monitor::Resource* res);

// NodeExt + 资源 -> NodeMetrics（/node/metrics 用）
// bmc_sensors 可选，如果提供则填充 latest_sensor_metrics
NodeMetrics toNodeMetrics(const node::NodeExt& ext, const monitor::Resource* res, std::int64_t now_seconds,
                          const std::unordered_map<std::string, bmc::BMCSensorRow>* bmc_sensors = nullptr);

// ResourceWindow + 可选BMC传感器 -> HistoricalMetricsView
HistoricalMetricsView toHistoricalMetricsView(const monitor::ResourceWindow& win,
                                              const nlohmann::json* bmc_sensor);

// 直接内联实现，避免链接符号不一致
inline HistoricalMetricsView toHistoricalMetricsView(const monitor::ResourceWindow& win,
                                                     const nlohmann::json* bmc_sensor) {
    HistoricalMetricsView v;
    v.box_id = win.box_id;
    v.cpu_id = win.cpu_id;
    v.host_ip = win.host_ip;
    v.slot_id = win.slot_id;
    v.time_range = win.time_range;
    v.metrics.cpu = win.metrics.cpu;
    v.metrics.disk = win.metrics.disk;
    v.metrics.gpu = win.metrics.gpu;
    v.metrics.network = win.metrics.network;
    v.metrics.memory = win.metrics.memory;
    if (bmc_sensor) v.metrics.sensor = *bmc_sensor; else v.metrics.sensor = nlohmann::json::object();
    return v;
}

} // namespace mapper
} // namespace web
} // namespace yw




