// ============================================================================
// 文件功能描述：
// 节点模型（node_model）的头文件，定义节点相关的数据模型和结构。
// 主要功能包括：
// 1. 节点数据模型：定义Node结构体，表示计算节点的完整信息（机箱号、槽位号、IP、类型等）
// 2. 节点扩展模型：定义NodeExt结构体，包含节点信息和最后更新时间戳
// 3. GPU设备模型：定义GpuDevice结构体，表示GPU设备的基本信息
// 4. JSON序列化：提供JSON序列化和反序列化支持，便于数据持久化和API交互
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace yw {
namespace node {

struct GpuDevice {
    int index = 0;
    std::string name;
};

struct Node {
    int box_id = 0;
    int slot_id = 0;
    int cpu_id = 0;
    int srio_id = 0;
    std::string host_ip;
    std::string hostname;
    uint16_t service_port = 0;
    std::string box_type;
    std::string board_type;
    std::string cpu_type;
    std::string os_type;
    std::string resource_type;
    std::string cpu_arch;
    std::vector<GpuDevice> gpu;
    std::string manufacturer;
    std::string serial_number;
    std::string production_date;
};

// 节点扩展信息（包含状态和时间戳）
struct NodeExt {
    int box_id = 0;
    int slot_id = 0;
    int cpu_id = 0;
    int srio_id = 0;
    std::string host_ip;
    std::string hostname;
    uint16_t service_port = 0;
    std::string box_type;
    std::string board_type;
    std::string cpu_type;
    std::string os_type;
    std::string resource_type;
    std::string cpu_arch;
    std::vector<GpuDevice> gpu;
    std::string manufacturer;
    std::string serial_number;
    std::string production_date;
    std::int64_t updated_at = 0;  // 最后更新时间（毫秒）
    std::string   status;         // 节点状态

    NodeExt() = default;
    explicit NodeExt(const Node& n, std::int64_t ts_ms)
        : box_id(n.box_id), slot_id(n.slot_id), cpu_id(n.cpu_id), srio_id(n.srio_id),
          host_ip(n.host_ip), hostname(n.hostname), service_port(n.service_port),
          box_type(n.box_type), board_type(n.board_type), cpu_type(n.cpu_type), os_type(n.os_type),
          resource_type(n.resource_type), cpu_arch(n.cpu_arch), gpu(n.gpu), manufacturer(n.manufacturer), serial_number(n.serial_number), production_date(n.production_date), updated_at(ts_ms) {}
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuDevice, index, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Node,
    box_id, slot_id, cpu_id, srio_id, host_ip, hostname, service_port,
    box_type, board_type, cpu_type, os_type, resource_type, cpu_arch, gpu, manufacturer, serial_number, production_date)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeExt,
    box_id, slot_id, cpu_id, srio_id, host_ip, hostname, service_port,
    box_type, board_type, cpu_type, os_type, resource_type, cpu_arch, gpu, manufacturer, serial_number, production_date,
    updated_at, status)

} // namespace node
} // namespace yw
