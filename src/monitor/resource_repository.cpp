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

    // Alive 心跳
    tx.exec_params(
        "INSERT INTO resource_alive(time, host_ip, alive) VALUES (now(), $1::inet, 1)",
        data.host_ip
    );

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

    // CPU - 每10秒聚合平均值
    if (query_all || need.count("cpu")) {
        pqxx::result r = tx.exec_params(
            // 2. 外层查询：负责格式化时间戳、处理NULL值和排序
            "SELECT "
            "    EXTRACT(EPOCH FROM bucket)::bigint AS ts, "
            "    COALESCE(usage_percent, 0) as usage_percent, "
            "    COALESCE(load_avg_1m, 0) as load_avg_1m, "
            "    COALESCE(load_avg_5m, 0) as load_avg_5m, "
            "    COALESCE(load_avg_15m, 0) as load_avg_15m, "
            "    COALESCE(core_count, 0)::int as core_count, "
            "    COALESCE(core_allocated, 0)::int as core_allocated, "
            "    COALESCE(temperature, 0) as temperature, "
            "    COALESCE(voltage, 0) as voltage, "
            "    COALESCE(current, 0) as current, "
            "    COALESCE(power, 0) as power "
            "FROM ( "
            // 1. 内层查询：只调用一次 time_bucket_gapfill，并完成所有聚合计算
            "    SELECT "
            "        time_bucket_gapfill('10 seconds', time, now() - $2::interval, now()) AS bucket, "
            "        AVG(usage_percent) as usage_percent, "
            "        AVG(load_avg_1m) as load_avg_1m, "
            "        AVG(load_avg_5m) as load_avg_5m, "
            "        AVG(load_avg_15m) as load_avg_15m, "
            "        ROUND(AVG(core_count)) as core_count, "
            "        ROUND(AVG(core_allocated)) as core_allocated, "
            "        AVG(temperature) as temperature, "
            "        AVG(voltage) as voltage, "
            "        AVG(current) as current, "
            "        AVG(power) as power "
            "    FROM resource_cpu "
            "    WHERE host_ip = $1::inet AND time >= now() - $2::interval AND time <= now() "
            "    GROUP BY bucket " // 直接按内层定义的别名 bucket 分组
            ") AS gapfilled_data " // 给子查询起一个别名
            "ORDER BY ts ASC",
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

        // delete the first and last element
        out.cpu.erase(out.cpu.begin());
        out.cpu.erase(out.cpu.end() - 1);
    }

    // Memory - 每10秒聚合平均值
    if (query_all || need.count("memory")) {
        pqxx::result r = tx.exec_params(
            "SELECT "
            "    EXTRACT(EPOCH FROM bucket)::bigint AS ts, "
            "    COALESCE(total, 0)::bigint as total, "
            "    COALESCE(used, 0)::bigint as used, "
            "    COALESCE(free, 0)::bigint as free, "
            "    COALESCE(usage_percent, 0) as usage_percent "
            "FROM ( "
            "    SELECT "
            "        time_bucket_gapfill('10 seconds', time, now() - $2::interval, now()) AS bucket, "
            "        ROUND(AVG(total)) as total, "
            "        ROUND(AVG(used)) as used, "
            "        ROUND(AVG(free)) as free, "
            "        AVG(usage_percent) as usage_percent "
            "    FROM resource_memory "
            "    WHERE host_ip = $1::inet AND time >= now() - $2::interval AND time <= now() "
            "    GROUP BY bucket "
            ") AS gapfilled_data "
            "ORDER BY ts ASC",
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

        // delete the first and last element
        out.memory.erase(out.memory.begin());
        out.memory.erase(out.memory.end() - 1);
    }

    // Network - 每10秒聚合平均值，按 interface 分组
    if (query_all || need.count("network")) {
        const char* network_query = R"SQL(
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
interface_dims AS (
  SELECT DISTINCT interface FROM resource_network
  WHERE host_ip = $1::inet
    AND time >= now() - $2::interval AND time <= now()
)
SELECT
    dims.interface,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    COALESCE(ROUND(AVG(metrics.rx_bytes)), 0)::bigint AS rx_bytes,
    COALESCE(ROUND(AVG(metrics.tx_bytes)), 0)::bigint AS tx_bytes,
    COALESCE(ROUND(AVG(metrics.rx_packets)), 0)::bigint AS rx_packets,
    COALESCE(ROUND(AVG(metrics.tx_packets)), 0)::bigint AS tx_packets,
    COALESCE(ROUND(AVG(metrics.rx_errors)), 0)::bigint AS rx_errors,
    COALESCE(ROUND(AVG(metrics.tx_errors)), 0)::bigint AS tx_errors,
    COALESCE(ROUND(AVG(metrics.rx_rate)), 0)::bigint AS rx_rate,
    COALESCE(ROUND(AVG(metrics.tx_rate)), 0)::bigint AS tx_rate
FROM
    interface_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    resource_network AS metrics
ON
    metrics.interface = dims.interface
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics.time) = buckets.bucket
    AND metrics.time >= now() - $2::interval AND metrics.time <= now()
GROUP BY
    dims.interface, buckets.bucket
ORDER BY
    dims.interface, ts ASC
)SQL";
        pqxx::result r = tx.exec_params(network_query, host_ip, duration);
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

        for (auto& [iface, points] : out.network) {
            points.erase(points.begin());
            points.erase(points.end() - 1);
        }
    }

    // Disk - 每10秒聚合平均值，按 device key 分组
    if (query_all || need.count("disk")) {
        const char* disk_query = R"SQL(
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
disk_dims AS (
  SELECT DISTINCT device, mount_point FROM resource_disk
  WHERE host_ip = $1::inet
    AND time >= now() - $2::interval AND time <= now()
)
SELECT
    dims.device,
    dims.mount_point,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    COALESCE(ROUND(AVG(metrics.total)), 0)::bigint AS total,
    COALESCE(ROUND(AVG(metrics.used)), 0)::bigint AS used,
    COALESCE(ROUND(AVG(metrics.free)), 0)::bigint AS free,
    COALESCE(AVG(metrics.usage_percent), 0) AS usage_percent
FROM
    disk_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    resource_disk AS metrics
ON
    metrics.device = dims.device
    AND metrics.mount_point = dims.mount_point
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics.time) = buckets.bucket
    AND metrics.time >= now() - $2::interval AND metrics.time <= now()
GROUP BY
    dims.device, dims.mount_point, buckets.bucket
ORDER BY
   dims.device, ts ASC
)SQL"; 
        pqxx::result r = tx.exec_params(disk_query, host_ip, duration);
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

        for (auto& [device, points] : out.disk) {
            points.erase(points.begin());
            points.erase(points.end() - 1);
        }
    }

    // GPU - 每10秒聚合平均值，按 gpu_index 分组
    if (query_all || need.count("gpu")) {
        const char* gpu_query = R"SQL(
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
gpu_dims AS (
  SELECT DISTINCT gpu_index, name FROM resource_gpu
  WHERE host_ip = $1::inet
    AND time >= now() - $2::interval AND time <= now()
)
SELECT
    dims.gpu_index,
    dims.name,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    COALESCE(AVG(metrics.compute_usage), 0) AS compute_usage,
    COALESCE(AVG(metrics.mem_usage), 0) AS mem_usage,
    COALESCE(ROUND(AVG(metrics.mem_used)), 0)::bigint AS mem_used,
    COALESCE(ROUND(AVG(metrics.mem_total)), 0)::bigint AS mem_total,
    COALESCE(AVG(metrics.temperature), 0) AS temperature,
    COALESCE(AVG(metrics.power), 0) AS power
FROM
    gpu_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    resource_gpu AS metrics
ON
    metrics.gpu_index = dims.gpu_index
    AND metrics.name = dims.name
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics.time) = buckets.bucket
    AND metrics.time >= now() - $2::interval AND metrics.time <= now()
GROUP BY
    dims.gpu_index, dims.name, buckets.bucket
ORDER BY
    dims.gpu_index, ts ASC
)SQL";
        pqxx::result r = tx.exec_params(gpu_query, host_ip, duration);
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

        for (auto& [key, points] : out.gpu) {
            points.erase(points.begin());
            points.erase(points.end() - 1);
        }
    }

    return out;
}

} // namespace monitor
} // namespace yw


