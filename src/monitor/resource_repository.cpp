#include "resource_repository.h"
#include <pqxx/pqxx>
#include <chrono>

namespace yw {
namespace monitor {

using clock = std::chrono::system_clock;

ResourceRepository::ResourceRepository(const std::string& conninfo)
    : conninfo_(conninfo) {}

void ResourceRepository::save(const Resource& data) {
    const auto now = clock::now();
    const auto now_ts = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const std::time_t t = clock::to_time_t(now_ts);

    pqxx::connection c(conninfo_);
    pqxx::work tx{c};

    // CPU
    tx.exec_params(
        "INSERT INTO resource_cpu(time, host_ip, usage_percent, load_avg_1m, load_avg_5m, load_avg_15m, core_count, core_allocated, temperature, voltage, current, power)"
        " VALUES (to_timestamp($1), $2::inet, $3,$4,$5,$6,$7,$8,$9,$10,$11,$12)",
        static_cast<long long>(t), data.host_ip,
        data.resource.cpu.usage_percent,
        data.resource.cpu.load_avg_1m,
        data.resource.cpu.load_avg_5m,
        data.resource.cpu.load_avg_15m,
        data.resource.cpu.core_count,
        data.resource.cpu.core_allocated,
        data.resource.cpu.temperature,
        data.resource.cpu.voltage,
        data.resource.cpu.current,
        data.resource.cpu.power
    );

    // Memory
    tx.exec_params(
        "INSERT INTO resource_memory(time, host_ip, total, used, free, usage_percent)"
        " VALUES (to_timestamp($1), $2::inet, $3,$4,$5,$6)",
        static_cast<long long>(t), data.host_ip,
        data.resource.memory.total,
        data.resource.memory.used,
        data.resource.memory.free,
        data.resource.memory.usage_percent
    );

    // Network
    for (const auto& nic : data.resource.network) {
        tx.exec_params(
            "INSERT INTO resource_network(time, host_ip, interface, rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_rate, tx_rate)"
            " VALUES (to_timestamp($1), $2::inet, $3,$4,$5,$6,$7,$8,$9,$10,$11)",
            static_cast<long long>(t), data.host_ip,
            nic.interface,
            nic.rx_bytes, nic.tx_bytes,
            nic.rx_packets, nic.tx_packets,
            nic.rx_errors, nic.tx_errors,
            nic.rx_rate, nic.tx_rate
        );
    }

    // Disk
    for (const auto& d : data.resource.disk) {
        tx.exec_params(
            "INSERT INTO resource_disk(time, host_ip, device, mount_point, total, used, free, usage_percent)"
            " VALUES (to_timestamp($1), $2::inet, $3,$4,$5,$6,$7,$8)",
            static_cast<long long>(t), data.host_ip,
            d.device, d.mount_point,
            d.total, d.used, d.free, d.usage_percent
        );
    }

    // GPU（节点级冗余字段可一起存）
    for (const auto& g : data.resource.gpu) {
        tx.exec_params(
            "INSERT INTO resource_gpu(time, host_ip, gpu_index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power, gpu_allocated, gpu_num)"
            " VALUES (to_timestamp($1), $2::inet, $3,$4,$5,$6,$7,$8,$9,$10,$11,$12)",
            static_cast<long long>(t), data.host_ip,
            g.index, g.name,
            g.compute_usage, g.mem_usage,
            g.mem_used, g.mem_total,
            g.temperature, g.power,
            data.resource.gpu_allocated, data.resource.gpu_num
        );
    }

    // Component
    for (const auto& comp : data.component) {
        tx.exec_params(
            "INSERT INTO component_resource(time, host_ip, instance_id, uuid, idx, name, container_id, state, cpu_load, mem_used, mem_limit, net_tx, net_rx)"
            " VALUES (to_timestamp($1), $2::inet, $3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)",
            static_cast<long long>(t), data.host_ip,
            comp.instance_id, comp.uuid, comp.index,
            comp.config.name, comp.config.id, comp.state,
            comp.resource.cpu.load,
            comp.resource.memory.mem_used, comp.resource.memory.mem_limit,
            comp.resource.network.tx, comp.resource.network.rx
        );
    }

    tx.commit();
}

} // namespace monitor
} // namespace yw


