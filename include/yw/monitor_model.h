#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace yw {
namespace monitor {

// --------------------
// 资源明细 - 主机级别
// --------------------

struct CpuResource {
    double usage_percent = 0.0;
    double load_avg_1m = 0.0;
    double load_avg_5m = 0.0;
    double load_avg_15m = 0.0;
    int    core_count = 0;
    int    core_allocated = 0;
    double temperature = 0.0;
    double voltage = 0.0;
    double current = 0.0;
    double power = 0.0;
};

struct MemoryResource {
    std::uint64_t total = 0;
    std::uint64_t used = 0;
    std::uint64_t free = 0;
    double        usage_percent = 0.0;
};

struct NetworkInterface {
    std::string   interface;
    std::uint64_t rx_bytes = 0;
    std::uint64_t tx_bytes = 0;
    std::uint64_t rx_packets = 0;
    std::uint64_t tx_packets = 0;
    std::uint64_t rx_errors = 0;
    std::uint64_t tx_errors = 0;
    std::uint64_t rx_rate = 0;
    std::uint64_t tx_rate = 0;
};

struct DiskPartition {
    std::string   device;
    std::string   mount_point;
    std::uint64_t total = 0;
    std::uint64_t used = 0;
    std::uint64_t free = 0;
    double        usage_percent = 0.0;
};

struct GpuResource {
    int           index = 0;
    std::string   name;
    double        compute_usage = 0.0;
    double        mem_usage = 0.0;
    std::uint64_t mem_used = 0;
    std::uint64_t mem_total = 0;
    double        temperature = 0.0;
    double        power = 0.0;
};

struct NodeResource {
    CpuResource                    cpu;
    MemoryResource                 memory;
    std::vector<NetworkInterface>  network;
    std::vector<DiskPartition>     disk;
    std::vector<GpuResource>       gpu;
    int                            gpu_allocated = 0;
    int                            gpu_num = 0;
};

// --------------------
// 资源明细 - 组件级别
// --------------------

struct ComponentCpuResource { double load = 0.0; };
struct ComponentMemoryResource { std::uint64_t mem_used = 0; std::uint64_t mem_limit = 0; };
struct ComponentNetworkResource { std::uint64_t tx = 0; std::uint64_t rx = 0; };

struct ComponentResource {
    ComponentCpuResource     cpu;
    ComponentMemoryResource  memory;
    ComponentNetworkResource network;
};

struct ComponentConfig { std::string name; std::string id; };

struct ComponentEntry {
    std::string     instance_id;
    std::string     uuid;
    int             index = 0;
    ComponentConfig config;
    std::string     state;
    ComponentResource resource;
};

// --------------------
// 顶层：/resource 的 data 字段
// --------------------

struct Resource {
    std::string                   host_ip;
    NodeResource                  resource;
    std::vector<ComponentEntry>   component;
};

// JSON 映射
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CpuResource,
    usage_percent, load_avg_1m, load_avg_5m, load_avg_15m,
    core_count, core_allocated, temperature, voltage, current, power)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MemoryResource,
    total, used, free, usage_percent)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetworkInterface,
    interface, rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_rate, tx_rate)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DiskPartition,
    device, mount_point, total, used, free, usage_percent)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuResource,
    index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeResource,
    cpu, memory, network, disk, gpu, gpu_allocated, gpu_num)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentCpuResource,
    load)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentMemoryResource,
    mem_used, mem_limit)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentNetworkResource,
    tx, rx)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentResource,
    cpu, memory, network)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentConfig,
    name, id)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentEntry,
    instance_id, uuid, index, config, state, resource)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Resource,
    host_ip, resource, component)

// --------------------
// 窗口指标序列（简化版，保持与现有实现一致）
// --------------------

struct CpuPoint {
    std::int64_t timestamp = 0; double usage_percent = 0.0; double load_avg_1m = 0.0; double load_avg_5m = 0.0; double load_avg_15m = 0.0; int core_count = 0; int core_allocated = 0; double temperature = 0.0; double voltage = 0.0; double current = 0.0; double power = 0.0;
};
struct MemoryPoint { std::int64_t timestamp = 0; std::uint64_t total = 0; std::uint64_t used = 0; std::uint64_t free = 0; double usage_percent = 0.0; };
struct NetworkPoint { std::int64_t timestamp = 0; std::string interface; std::uint64_t rx_bytes = 0; std::uint64_t tx_bytes = 0; std::uint64_t rx_packets = 0; std::uint64_t tx_packets = 0; std::uint64_t rx_errors = 0; std::uint64_t tx_errors = 0; std::uint64_t rx_rate = 0; std::uint64_t tx_rate = 0; };
struct DiskPoint { std::int64_t timestamp = 0; std::string device; std::string mount_point; std::uint64_t total = 0; std::uint64_t used = 0; std::uint64_t free = 0; double usage_percent = 0.0; };
struct GpuPoint { std::int64_t timestamp = 0; int index = 0; std::string name; double compute_usage = 0.0; double mem_usage = 0.0; std::uint64_t mem_used = 0; std::uint64_t mem_total = 0; double temperature = 0.0; double power = 0.0; };

struct MetricsSeries {
    std::vector<CpuPoint> cpu;
    std::unordered_map<std::string, std::vector<DiskPoint>> disk;
    std::unordered_map<std::string, std::vector<GpuPoint>>  gpu;
    std::unordered_map<std::string, std::vector<NetworkPoint>> network;
    std::vector<MemoryPoint> memory;
};

struct ResourceWindow {
    int box_id = 0; int cpu_id = 0; std::string host_ip; int slot_id = 0; std::string time_range; MetricsSeries metrics;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CpuPoint,
    timestamp, usage_percent, load_avg_1m, load_avg_5m, load_avg_15m, core_count, core_allocated, temperature, voltage, current, power)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MemoryPoint,
    timestamp, total, used, free, usage_percent)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetworkPoint,
    interface, rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_rate, tx_rate, timestamp)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DiskPoint,
    device, mount_point, total, used, free, usage_percent, timestamp)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuPoint,
    index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power, timestamp)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MetricsSeries,
    cpu, disk, gpu, network, memory)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceWindow,
    box_id, cpu_id, host_ip, metrics, slot_id, time_range)

} // namespace monitor
} // namespace yw


