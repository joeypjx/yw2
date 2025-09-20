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
    std::int64_t updated_at = 0;
    std::string   status;

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
