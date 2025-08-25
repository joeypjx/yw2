#include "resource_repository.h"
#include <pqxx/pqxx>
#include <unordered_set>

namespace yw {
namespace monitor {

 

ResourceRepository::ResourceRepository(const std::string& conninfo)
    : conninfo_(conninfo) {}

void ResourceRepository::save(const Resource& data) {
    pqxx::connection c(conninfo_);
    pqxx::work tx{c};

    // CPU
    tx.exec_params(
        "INSERT INTO resource_cpu(time, host_ip, usage_percent, load_avg_1m, load_avg_5m, load_avg_15m, core_count, core_allocated, temperature, voltage, current, power)"
        " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10,$11)",
        data.host_ip,
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
        " VALUES (now(), $1::inet, $2,$3,$4,$5)",
        data.host_ip,
        data.resource.memory.total,
        data.resource.memory.used,
        data.resource.memory.free,
        data.resource.memory.usage_percent
    );

    // Network
    for (const auto& nic : data.resource.network) {
        tx.exec_params(
            "INSERT INTO resource_network(time, host_ip, interface, rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_rate, tx_rate)"
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10)",
            data.host_ip,
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
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7)",
            data.host_ip,
            d.device, d.mount_point,
            d.total, d.used, d.free, d.usage_percent
        );
    }

    // GPU（节点级冗余字段可一起存）
    for (const auto& g : data.resource.gpu) {
        tx.exec_params(
            "INSERT INTO resource_gpu(time, host_ip, gpu_index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power, gpu_allocated, gpu_num)"
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10,$11)",
            data.host_ip,
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
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)",
            data.host_ip,
            comp.instance_id, comp.uuid, comp.index,
            comp.config.name, comp.config.id, comp.state,
            comp.resource.cpu.load,
            comp.resource.memory.mem_used, comp.resource.memory.mem_limit,
            comp.resource.network.tx, comp.resource.network.rx
        );
    }

    tx.commit();
}

MetricsSeries ResourceRepository::queryMetricsSeries(const std::string& host_ip,
                                                     const std::string& duration,
                                                     const std::vector<std::string>& kinds) {
    MetricsSeries out;
    std::unordered_set<std::string> need(kinds.begin(), kinds.end());
    const bool query_all = need.empty();

    pqxx::connection c(conninfo_);
    pqxx::read_transaction tx{c};

    // CPU
    if (query_all || need.count("cpu")) {
        pqxx::result r = tx.exec_params(
            "SELECT EXTRACT(EPOCH FROM time)::bigint AS ts, usage_percent, load_avg_1m, load_avg_5m, load_avg_15m,"
            " core_count, core_allocated, temperature, voltage, current, power"
            " FROM resource_cpu"
            " WHERE host_ip = $1::inet AND time >= now() - $2::interval"
            " ORDER BY time ASC",
            host_ip, duration
        );
        out.cpu.reserve(r.size());
        for (const auto& row : r) {
            CpuPoint p{};
            p.timestamp      = row[0].as<long long>(0);
            p.usage_percent  = row[1].as<double>(0);
            p.load_avg_1m    = row[2].as<double>(0);
            p.load_avg_5m    = row[3].as<double>(0);
            p.load_avg_15m   = row[4].as<double>(0);
            p.core_count     = row[5].as<int>(0);
            p.core_allocated = row[6].as<int>(0);
            p.temperature    = row[7].as<double>(0);
            p.voltage        = row[8].as<double>(0);
            p.current        = row[9].as<double>(0);
            p.power          = row[10].as<double>(0);
            out.cpu.push_back(std::move(p));
        }
    }

    // Memory
    if (query_all || need.count("memory")) {
        pqxx::result r = tx.exec_params(
            "SELECT EXTRACT(EPOCH FROM time)::bigint AS ts, total, used, free, usage_percent"
            " FROM resource_memory"
            " WHERE host_ip = $1::inet AND time >= now() - $2::interval"
            " ORDER BY time ASC",
            host_ip, duration
        );
        out.memory.reserve(r.size());
        for (const auto& row : r) {
            MemoryPoint p{};
            p.timestamp     = row[0].as<long long>(0);
            p.total         = row[1].as<long long>(0);
            p.used          = row[2].as<long long>(0);
            p.free          = row[3].as<long long>(0);
            p.usage_percent = row[4].as<double>(0);
            out.memory.push_back(std::move(p));
        }
    }

    // Network (按 interface 分组)
    if (query_all || need.count("network")) {
        pqxx::result r = tx.exec_params(
            "SELECT interface, EXTRACT(EPOCH FROM time)::bigint AS ts, rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_rate, tx_rate"
            " FROM resource_network"
            " WHERE host_ip = $1::inet AND time >= now() - $2::interval"
            " ORDER BY interface, time ASC",
            host_ip, duration
        );
        for (const auto& row : r) {
            NetworkPoint p{};
            const std::string iface = row[0].as<std::string>("");
            p.timestamp   = row[1].as<long long>(0);
            p.interface   = iface;
            p.rx_bytes    = row[2].as<long long>(0);
            p.tx_bytes    = row[3].as<long long>(0);
            p.rx_packets  = row[4].as<long long>(0);
            p.tx_packets  = row[5].as<long long>(0);
            p.rx_errors   = row[6].as<long long>(0);
            p.tx_errors   = row[7].as<long long>(0);
            p.rx_rate     = row[8].as<long long>(0);
            p.tx_rate     = row[9].as<long long>(0);
            out.network[iface].push_back(std::move(p));
        }
    }

    // Disk (按 device key 分组)
    if (query_all || need.count("disk")) {
        pqxx::result r = tx.exec_params(
            "SELECT device, mount_point, EXTRACT(EPOCH FROM time)::bigint AS ts, total, used, free, usage_percent"
            " FROM resource_disk"
            " WHERE host_ip = $1::inet AND time >= now() - $2::interval"
            " ORDER BY device, time ASC",
            host_ip, duration
        );
        for (const auto& row : r) {
            DiskPoint p{};
            const std::string device = row[0].as<std::string>("");
            p.device       = device;
            p.mount_point  = row[1].as<std::string>("");
            p.timestamp    = row[2].as<long long>(0);
            p.total        = row[3].as<long long>(0);
            p.used         = row[4].as<long long>(0);
            p.free         = row[5].as<long long>(0);
            p.usage_percent= row[6].as<double>(0);
            out.disk[device].push_back(std::move(p));
        }
    }

    // GPU (按 gpu_index 分组)
    if (query_all || need.count("gpu")) {
        pqxx::result r = tx.exec_params(
            "SELECT gpu_index, name, EXTRACT(EPOCH FROM time)::bigint AS ts, compute_usage, mem_usage, mem_used, mem_total, temperature, power"
            " FROM resource_gpu"
            " WHERE host_ip = $1::inet AND time >= now() - $2::interval"
            " ORDER BY gpu_index, time ASC",
            host_ip, duration
        );
        for (const auto& row : r) {
            GpuPoint p{};
            const int index = row[0].as<int>(0);
            p.index         = index;
            p.name          = row[1].as<std::string>("");
            p.timestamp     = row[2].as<long long>(0);
            p.compute_usage = row[3].as<double>(0);
            p.mem_usage     = row[4].as<double>(0);
            p.mem_used      = row[5].as<long long>(0);
            p.mem_total     = row[6].as<long long>(0);
            p.temperature   = row[7].as<double>(0);
            p.power         = row[8].as<double>(0);
            std::string key = std::string("gpu_") + std::to_string(index);
            out.gpu[key].push_back(std::move(p));
        }
    }

    return out;
}

} // namespace monitor
} // namespace yw


