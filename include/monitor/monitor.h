#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "monitor/monitor_model.h"

// 前向声明
namespace hv { struct HttpService; }
namespace yw { namespace node { class INodeModule; } }

namespace yw {
namespace monitor {

struct Resource; // 前向声明，避免在公共头包含私有实现头

// 导出数据结构
struct ExportDataPoint {
    std::string timestamp;  // 格式: "2024-01-01 00:00:00"
    double cpu_usage_percent = 0.0;
    double memory_usage_percent = 0.0;
    std::map<std::string, double> disk_usage_percent;  // key: mount_point, value: usage_percent
    std::map<std::string, double> network_rx_rate;     // key: interface, value: rx_rate
    std::map<std::string, double> network_tx_rate;     // key: interface, value: tx_rate
    std::map<std::string, double> gpu_compute_usage;   // key: gpu_index, value: compute_usage
    std::map<std::string, double> gpu_mem_usage;        // key: gpu_index, value: mem_usage
};

struct ExportData {
    std::string start_time;  // 格式: "2024-01-01 00:00:00"
    std::string end_time;    // 格式: "2024-01-01 00:00:00"
    std::string ip;
    std::vector<std::string> type;  // 包含的指标类型
    std::vector<ExportDataPoint> data;
};

class IMonitorModule {
public:
    virtual ~IMonitorModule() = default;
    // 根据节点IP返回最近一次上报的资源快照；未命中返回 nullptr
    virtual std::shared_ptr<Resource> getNodeResource(const std::string& host_ip) const = 0;

    // 查询一段时间窗口内的指标序列（kinds: cpu/memory/network/disk/gpu 空表示全部）
    // duration 允许简写：1h/5m/10s（仅单一单位）
    virtual MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                             const std::string& duration,
                                             const std::vector<std::string>& kinds) const = 0;

    // 导出节点历史数据（按 export.md 格式）
    // start_time, end_time 为秒级时间戳
    // types 指定要导出的指标类型，为空表示全部
    virtual ExportData exportNodeData(const std::string& host_ip,
                                      std::int64_t start_time,
                                      std::int64_t end_time,
                                      const std::vector<std::string>& types) const = 0;
};

class MonitorFactory {
public:
    static std::shared_ptr<IMonitorModule> getMonitorModule(
        std::shared_ptr<hv::HttpService> service,
        std::shared_ptr<node::INodeModule> node_module);
};

} // namespace monitor
} // namespace yw


