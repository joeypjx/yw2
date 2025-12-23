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

struct MetricsSeriesView {
    std::vector<monitor::CpuPoint> cpu;
    std::unordered_map<std::string, std::vector<monitor::DiskPoint>> disk;
    std::unordered_map<std::string, std::vector<monitor::GpuPoint>>  gpu;
    std::unordered_map<std::string, std::vector<monitor::NetworkPoint>> network;
    std::vector<monitor::MemoryPoint> memory;
    nlohmann::json sensor = nlohmann::json::object();
};

struct HistoricalMetricsView {
    int         box_id = 0;
    int         cpu_id = 0;
    std::string host_ip;
    int         slot_id = 0;
    std::string time_range;
    MetricsSeriesView metrics;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MetricsSeriesView,
    cpu, disk, gpu, network, memory, sensor)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HistoricalMetricsView,
    box_id, cpu_id, host_ip, slot_id, time_range, metrics)

} // namespace web
} // namespace yw

