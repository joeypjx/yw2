// ============================================================================
// 文件功能描述：
// 节点视图DTO（node_view_dto）的头文件，定义节点列表API响应的数据传输对象。
// 主要功能包括：
// 1. 节点视图定义：定义NodeView结构体，用于/node API的响应数据
// 2. GPU设备信息：定义GpuDevice结构体，表示GPU设备的基本信息
// 3. JSON序列化：提供JSON序列化和反序列化支持，便于API响应格式化
// 4. 数据转换：将内部数据模型转换为前端友好的格式
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "monitor/monitor_model.h" // 复用 ComponentEntry 的定义与 JSON 宏

namespace yw {
namespace web {

// --------------------
// /node 列表视图 DTO
// --------------------

// GPU设备信息
struct GpuDevice {
    int index = 0;
    std::string name;
};

// 节点列表视图数据，用于 /node API 响应
struct NodeView {
    int          box_id = 0;
    int          slot_id = 0;
    int          cpu_id = 0;
    std::string  host_ip;
    std::string  hostname;
    std::string  status;
    std::string  box_type;
    std::string  board_type;
    std::string  cpu_type;
    std::string  os_type;
    std::string  resource_type;
    std::string  cpu_arch;
    std::string  manufacturer;
    std::string  serial_number;
    std::string  production_date;

    std::vector<GpuDevice> gpu;

    std::int64_t updated_at = 0;

    std::vector<monitor::ComponentEntry> component;
};

// /node 列表项视图 JSON 映射
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpuDevice, index, name)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeView,
    box_id, slot_id, cpu_id, host_ip, hostname, status, box_type, board_type, cpu_type, os_type, resource_type, cpu_arch, gpu, manufacturer, serial_number, production_date, updated_at, component)

} // namespace web
} // namespace yw

