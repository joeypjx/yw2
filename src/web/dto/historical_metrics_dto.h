// ============================================================================
// 文件功能描述：
// 历史指标DTO（historical_metrics_dto）的头文件，定义节点历史指标API响应的数据传输对象。
// 主要功能包括：
// 1. 历史指标视图定义：定义HistoricalMetricsView结构体，用于/node/historical-metrics API的响应数据
// 2. 指标时序数据视图：定义MetricsSeriesView结构体，包含各种资源类型的时序点
// 3. JSON序列化：提供JSON序列化和反序列化支持，便于API响应格式化
// 4. 数据转换：将内部数据模型转换为前端友好的格式
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "monitor/monitor_model.h" // 复用 CpuPoint, DiskPoint 等定义

namespace yw {
namespace web {

// --------------------
// /node/historical-metrics 视图 DTO
// --------------------

// 指标时序数据视图，包含各种资源类型的时序点（用于/node/historical-metrics API响应）
struct MetricsSeriesView {
    std::vector<monitor::CpuPoint> cpu;
    std::unordered_map<std::string, std::vector<monitor::DiskPoint>> disk;
    std::unordered_map<std::string, std::vector<monitor::GpuPoint>>  gpu;
    std::unordered_map<std::string, std::vector<monitor::NetworkPoint>> network;
    std::vector<monitor::MemoryPoint> memory;
    nlohmann::json sensor = nlohmann::json::object();
};

// 历史指标视图，包含节点信息和指标时序数据（用于/node/historical-metrics API响应）
struct HistoricalMetricsView {
    int         box_id = 0;
    int         cpu_id = 0;
    std::string host_ip;
    int         slot_id = 0;
    std::string time_range;
    MetricsSeriesView metrics;
};

// 定义MetricsSeriesView结构的JSON序列化/反序列化
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MetricsSeriesView,
    cpu, disk, gpu, network, memory, sensor)

// 定义HistoricalMetricsView结构的JSON序列化/反序列化
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HistoricalMetricsView,
    box_id, cpu_id, host_ip, slot_id, time_range, metrics)

} // namespace web
} // namespace yw

