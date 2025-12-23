#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "monitor/monitor_model.h" // 复用 ComponentEntry 的定义与 JSON 宏

namespace yw {
namespace web {

// --------------------
// latest_* 明细结构
// --------------------

struct LatestContainerMetrics {
    int         container_count = 0;
    int         paused_count = 0;
    int         running_count = 0;
    int         stopped_count = 0;
    std::int64_t timestamp = 0;
};

struct LatestCpuMetrics {
    int          core_allocated = 0;
    int          core_count = 0;
    double       current = 0.0;
    double       load_avg_15m = 0.0;
    double       load_avg_1m = 0.0;
    double       load_avg_5m = 0.0;
    double       power = 0.0;
    double       temperature = 0.0;
    std::int64_t timestamp = 0;
    double       usage_percent = 0.0;
    double       voltage = 0.0;
};

struct DiskSample {
    std::string   device;
    std::uint64_t free = 0;
    std::string   mount_point;
    std::int64_t  timestamp = 0;
    std::uint64_t total = 0;
    double        usage_percent = 0.0;
    std::uint64_t used = 0;
};

struct LatestDiskMetrics {
    int                    disk_count = 0;
    std::vector<DiskSample> disks;
    std::int64_t           timestamp = 0;
};

struct GpuSample {
    double       compute_usage = 0.0;
    double       current = 0.0;
    int          index = 0;
    std::uint64_t mem_total = 0;
    double       mem_usage = 0.0;
    std::uint64_t mem_used = 0;
    std::string  name;
    double       power = 0.0;
    double       temperature = 0.0;
    std::int64_t timestamp = 0;
    double       voltage = 0.0;
};

struct LatestGpuMetrics {
    int                 gpu_count = 0;
    std::vector<GpuSample> gpus;
    std::int64_t        timestamp = 0;
};

struct LatestMemoryMetrics {
    std::uint64_t free = 0;
    std::int64_t  timestamp = 0;
    std::uint64_t total = 0;
    double        usage_percent = 0.0;
    std::uint64_t used = 0;
};

struct NetworkSample {
    std::string   interface;
    std::uint64_t rx_bytes = 0;
    std::uint64_t rx_errors = 0;
    std::uint64_t rx_packets = 0;
    double        rx_rate = 0.0;
    double        rx_drop_rate = 0.0;
    std::int64_t  timestamp = 0;
    std::uint64_t tx_bytes = 0;
    std::uint64_t tx_errors = 0;
    std::uint64_t tx_packets = 0;
    double        tx_rate = 0.0;
    double        tx_drop_rate = 0.0;
    int           state = 0;
};

struct LatestNetworkMetrics {
    int                     network_count = 0;
    std::vector<NetworkSample> networks;
    std::int64_t            timestamp = 0;
};

struct LatestSensorMetrics {
    int                              sensor_count = 0;
    std::vector<nlohmann::json>      sensors; // 结构未定义，保持原样透传
    std::int64_t                     timestamp = 0;
};

// --------------------
// 顶层：NodeMetrics
// --------------------

struct NodeMetrics {
    int                              bmc_company = 0;
    std::string                      bmc_version;
    std::string                      board_type;
    int                              box_id = 0;
    std::string                      box_type;
    std::vector<monitor::ComponentEntry> component;
    std::string                      cpu_arch;
    int                              cpu_id = 0;
    std::string                      cpu_type;
    std::string                      host_ip;
    std::string                      hostname;
    int                              id = 0;
    int                              ipmb_address = 0;
    LatestContainerMetrics           latest_container_metrics;
    LatestCpuMetrics                 latest_cpu_metrics;
    LatestDiskMetrics                latest_disk_metrics;
    LatestGpuMetrics                 latest_gpu_metrics;
    LatestMemoryMetrics              latest_memory_metrics;
    LatestNetworkMetrics             latest_network_metrics;
    LatestSensorMetrics              latest_sensor_metrics;
    int                              module_type = 0;
    std::string                      os_type;
    std::string                      resource_type;
    int                              service_port = 0;
    int                              slot_id = 0;
    int                              srio_id = 0;
    std::string                      status;
    std::int64_t                     updated_at = 0;
};

// --------------------
// JSON 映射
// --------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestContainerMetrics,
    container_count,
    paused_count,
    running_count,
    stopped_count,
    timestamp
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestCpuMetrics,
    core_allocated,
    core_count,
    current,
    load_avg_15m,
    load_avg_1m,
    load_avg_5m,
    power,
    temperature,
    timestamp,
    usage_percent,
    voltage
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DiskSample,
    device,
    free,
    mount_point,
    timestamp,
    total,
    usage_percent,
    used
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestDiskMetrics,
    disk_count,
    disks,
    timestamp
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuSample,
    compute_usage,
    current,
    index,
    mem_total,
    mem_usage,
    mem_used,
    name,
    power,
    temperature,
    timestamp,
    voltage
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestGpuMetrics,
    gpu_count,
    gpus,
    timestamp
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestMemoryMetrics,
    free,
    timestamp,
    total,
    usage_percent,
    used
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetworkSample,
    interface,
    rx_bytes,
    rx_errors,
    rx_packets,
    rx_rate,
    rx_drop_rate,
    timestamp,
    tx_bytes,
    tx_errors,
    tx_packets,
    tx_rate,
    tx_drop_rate,
    state
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestNetworkMetrics,
    network_count,
    networks,
    timestamp
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LatestSensorMetrics,
    sensor_count,
    sensors,
    timestamp
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeMetrics,
    bmc_company,
    bmc_version,
    board_type,
    box_id,
    box_type,
    component,
    cpu_arch,
    cpu_id,
    cpu_type,
    host_ip,
    hostname,
    id,
    ipmb_address,
    latest_container_metrics,
    latest_cpu_metrics,
    latest_disk_metrics,
    latest_gpu_metrics,
    latest_memory_metrics,
    latest_network_metrics,
    latest_sensor_metrics,
    module_type,
    os_type,
    resource_type,
    service_port,
    slot_id,
    srio_id,
    status,
    updated_at
)

} // namespace web
} // namespace yw

