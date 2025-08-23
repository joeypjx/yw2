#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace yw {
namespace monitor {

// --------------------
// 资源明细 - 主机级别
// --------------------

struct CpuResource {
    double usage_percent = 0.0;     // CPU使用率百分比
    double load_avg_1m = 0.0;       // 1分钟负载
    double load_avg_5m = 0.0;       // 5分钟负载
    double load_avg_15m = 0.0;      // 15分钟负载
    int    core_count = 0;          // CPU核数
    int    core_allocated = 0;      // 已分配核数
    double temperature = 0.0;       // 温度(摄氏度)
    double voltage = 0.0;           // 电压(伏特)
    double current = 0.0;           // 电流(安培)
    double power = 0.0;             // 功耗(瓦特)
};

struct MemoryResource {
    std::uint64_t total = 0;        // 总大小(字节)
    std::uint64_t used = 0;         // 已使用(字节)
    std::uint64_t free = 0;         // 剩余(字节)
    double        usage_percent = 0.0; // 使用率(%)
};

struct NetworkInterface {
    std::string   interface;        // 网卡名
    std::uint64_t rx_bytes = 0;     // 接收字节
    std::uint64_t tx_bytes = 0;     // 发送字节
    std::uint64_t rx_packets = 0;   // 接收报文
    std::uint64_t tx_packets = 0;   // 发送报文
    std::uint64_t rx_errors = 0;    // 接收错误
    std::uint64_t tx_errors = 0;    // 发送错误
    std::uint64_t rx_rate = 0;      // 接收速率(字节/秒)
    std::uint64_t tx_rate = 0;      // 发送速率(字节/秒)
};

struct DiskPartition {
    std::string   device;           // 设备名称
    std::string   mount_point;      // 挂载路径
    std::uint64_t total = 0;        // 总大小(字节)
    std::uint64_t used = 0;         // 已使用(字节)
    std::uint64_t free = 0;         // 剩余(字节)
    double        usage_percent = 0.0; // 使用率(%)
};

struct GpuResource {
    int           index = 0;        // 设备序号
    std::string   name;             // 设备名称
    double        compute_usage = 0.0; // 计算使用率(%)
    double        mem_usage = 0.0;  // 显存使用率(%)
    std::uint64_t mem_used = 0;     // 显存已使用(字节)
    std::uint64_t mem_total = 0;    // 显存总大小(字节)
    double        temperature = 0.0; // 温度(摄氏度)
    double        power = 0.0;      // 功耗(瓦特)
};

struct NodeResource {
    CpuResource                    cpu;            // CPU
    MemoryResource                 memory;         // 内存
    std::vector<NetworkInterface>  network;        // 网络
    std::vector<DiskPartition>     disk;           // 磁盘
    std::vector<GpuResource>       gpu;            // GPU列表
    int                            gpu_allocated = 0; // 已分配GPU数量
    int                            gpu_num = 0;       // GPU总数
};

// --------------------
// 资源明细 - 组件级别
// --------------------

struct ComponentCpuResource {
    double load = 0.0; // 负载率(%)
};

struct ComponentMemoryResource {
    std::uint64_t mem_used = 0;   // 使用(字节)
    std::uint64_t mem_limit = 0;  // 限制(字节)
};

struct ComponentNetworkResource {
    std::uint64_t tx = 0; // 发送(字节)
    std::uint64_t rx = 0; // 接收(字节)
};

struct ComponentResource {
    ComponentCpuResource     cpu;
    ComponentMemoryResource  memory;
    ComponentNetworkResource network;
};

struct ComponentConfig {
    std::string name; // 容器名
    std::string id;   // 容器ID(docker id)
};

struct ComponentEntry {
    std::string     instance_id; // 业务实例id
    std::string     uuid;        // 组件实例uuid
    int             index = 0;   // 索引
    ComponentConfig config;      // 配置
    std::string     state;       // 状态: PENDING/RUNNING/FAILED/STOPPED/SLEEPING
    ComponentResource resource;  // 资源
};

// --------------------
// 顶层数据结构: /resource 的 data 字段
// --------------------

struct Resource {
    std::string                   host_ip;    // 节点IP
    NodeResource                  resource;   // 主机资源
    std::vector<ComponentEntry>   component;  // 组件资源
};

// --------------------
// JSON 映射
// --------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CpuResource,
    usage_percent,
    load_avg_1m,
    load_avg_5m,
    load_avg_15m,
    core_count,
    core_allocated,
    temperature,
    voltage,
    current,
    power
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MemoryResource,
    total,
    used,
    free,
    usage_percent
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetworkInterface,
    interface,
    rx_bytes,
    tx_bytes,
    rx_packets,
    tx_packets,
    rx_errors,
    tx_errors,
    rx_rate,
    tx_rate
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DiskPartition,
    device,
    mount_point,
    total,
    used,
    free,
    usage_percent
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuResource,
    index,
    name,
    compute_usage,
    mem_usage,
    mem_used,
    mem_total,
    temperature,
    power
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeResource,
    cpu,
    memory,
    network,
    disk,
    gpu,
    gpu_allocated,
    gpu_num
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentCpuResource,
    load
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentMemoryResource,
    mem_used,
    mem_limit
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentNetworkResource,
    tx,
    rx
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentResource,
    cpu,
    memory,
    network
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentConfig,
    name,
    id
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentEntry,
    instance_id,
    uuid,
    index,
    config,
    state,
    resource
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Resource,
    host_ip,
    resource,
    component
)

} // namespace monitor
} // namespace yw


