#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace yw {
namespace node {

/**
 * @brief GPU设备信息结构
 */
struct GpuDevice {
    int index = 0;              // GPU设备序号
    std::string name;           // GPU设备名称
};

/**
 * @brief 节点信息结构，对应心跳接口中的SystemInfo数据结构
 */
struct Node {
    // 基本信息
    int box_id = 0;                 // 机箱号
    int slot_id = 0;                // 槽位号  
    int cpu_id = 0;                 // CPU号
    int srio_id = 0;                // SRIO号
    std::string host_ip;            // 主机IP地址
    std::string hostname;           // 主机名
    uint16_t service_port = 0;      // 命令响应服务端口

    // 硬件信息
    std::string box_type;           // 机箱类型
    std::string board_type;         // 板卡类型
    std::string cpu_type;           // CPU类型
    std::string os_type;            // 操作系统类型
    std::string resource_type;      // 资源类型
    std::string cpu_arch;           // CPU架构

    // GPU设备列表
    std::vector<GpuDevice> gpu;
};

/**
 * @brief 节点响应结构：在 Node 基础上增加更新时间字段（扁平结构）
 */
struct NodeExt {
    // Node 原有字段
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

    // 扩展元数据
    std::int64_t updated_at = 0;  // 最后更新时间（毫秒）

    NodeExt() = default;
    explicit NodeExt(const Node& n, std::int64_t ts_ms)
        : box_id(n.box_id),
          slot_id(n.slot_id),
          cpu_id(n.cpu_id),
          srio_id(n.srio_id),
          host_ip(n.host_ip),
          hostname(n.hostname),
          service_port(n.service_port),
          box_type(n.box_type),
          board_type(n.board_type),
          cpu_type(n.cpu_type),
          os_type(n.os_type),
          resource_type(n.resource_type),
          cpu_arch(n.cpu_arch),
          gpu(n.gpu),
          updated_at(ts_ms) {}
};

// nlohmann/json 宏定义，用于 JSON 序列化/反序列化
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuDevice,
    index,
    name
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Node,
    box_id,
    slot_id,
    cpu_id,
    srio_id,
    host_ip,
    hostname,
    service_port,
    box_type,
    board_type,
    cpu_type,
    os_type,
    resource_type,
    cpu_arch,
    gpu
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeExt,
    box_id,
    slot_id,
    cpu_id,
    srio_id,
    host_ip,
    hostname,
    service_port,
    box_type,
    board_type,
    cpu_type,
    os_type,
    resource_type,
    cpu_arch,
    gpu,
    updated_at
)

} // namespace node
} // namespace yw
