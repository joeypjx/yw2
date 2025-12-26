#include "resource_repository.h"
#include "utils/postgresql_connection_pool.h"
#include <pqxx/pqxx>
#include <unordered_set>

namespace yw {
namespace monitor {

ResourceRepository::ResourceRepository(const std::string& conninfo,
                                       size_t minConnections,
                                       size_t maxConnections)
    : connectionPool_(std::make_unique<yw::utils::PostgreSQLConnectionPool>(conninfo, minConnections, maxConnections)) {
}
 
ResourceRepository::~ResourceRepository() = default;

void ResourceRepository::save(const Resource& data) {
    // 从连接池获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    pqxx::work tx{*conn};

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
            "INSERT INTO resource_network(time, host_ip, interface, rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_rate, tx_rate, rx_drop_rate, tx_drop_rate, state)"
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)",
            data.host_ip,
            nic.interface,
            nic.rx_bytes, nic.tx_bytes,
            nic.rx_packets, nic.tx_packets,
            nic.rx_errors, nic.tx_errors,
            nic.rx_rate, nic.tx_rate,
            nic.rx_drop_rate, nic.tx_drop_rate,
            nic.state
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
            "INSERT INTO resource_gpu(time, host_ip, gpu_index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power, free, gpu_allocated, gpu_num)"
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)",
            data.host_ip,
            g.index, g.name,
            g.compute_usage, g.mem_usage,
            g.mem_used, g.mem_total,
            g.temperature, g.power,
            g.free,
            data.resource.gpu_allocated, data.resource.gpu_num
        );
    }

    // Component
    for (const auto& comp : data.component) {
        tx.exec_params(
            "INSERT INTO resource_component(time, host_ip, instance_id, uuid, idx, name, container_id, state, type, cpu_load, mem_used, mem_limit, mem_usage, net_tx, net_rx, net_rx_rate, net_tx_rate)"
            " VALUES (now(), $1::inet, $2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16)",
            data.host_ip,
            comp.instance_id, comp.uuid, comp.index,
            comp.config.name, comp.config.id, comp.state, comp.type,
            comp.resource.cpu.load,
            comp.resource.memory.mem_used, comp.resource.memory.mem_limit,
            comp.resource.memory.mem_usage,
            comp.resource.network.tx, comp.resource.network.rx,
            comp.resource.network.rx_rate, comp.resource.network.tx_rate
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

    // 从连接池获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    pqxx::read_transaction tx{*conn};

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
    COALESCE(AVG(metrics.power), 0) AS power,
    COALESCE(ROUND(AVG(metrics.free)), 0)::integer AS free
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
            p.free          = row[9].as<int>(0);
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

MetricsSeries ResourceRepository::queryMetricsSeries(const std::string& host_ip,
                                                     std::int64_t start_time,
                                                     std::int64_t end_time,
                                                     const std::vector<std::string>& kinds) {
    MetricsSeries out;
    std::unordered_set<std::string> need(kinds.begin(), kinds.end());
    const bool query_all = need.empty();

    // 从连接池获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    pqxx::read_transaction tx{*conn};

    // 根据时间范围动态调整 bucket 大小，避免生成过多bucket
    // 1小时以内：10秒，1-6小时：1分钟，6-24小时：5分钟，超过24小时：15分钟
    const std::int64_t duration_seconds = end_time - start_time;
    std::string bucket_interval;
    if (duration_seconds <= 3600) {
        bucket_interval = "'10 seconds'";
    } else if (duration_seconds <= 21600) {
        bucket_interval = "'1 minute'";
    } else if (duration_seconds <= 86400) {
        bucket_interval = "'5 minutes'";
    } else {
        bucket_interval = "'15 minutes'";
    }

    // CPU - 每10秒聚合平均值
    if (query_all || need.count("cpu")) {
        pqxx::result r = tx.exec_params(
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
            "    SELECT "
            "        time_bucket_gapfill(" + bucket_interval + ", time, to_timestamp($2), to_timestamp($3)) AS bucket, "
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
            "    WHERE host_ip = $1::inet AND time >= to_timestamp($2) AND time <= to_timestamp($3) "
            "    GROUP BY bucket "
            ") AS gapfilled_data "
            "ORDER BY ts ASC",
            host_ip, start_time, end_time
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
            "        time_bucket_gapfill(" + bucket_interval + ", time, to_timestamp($2), to_timestamp($3)) AS bucket, "
            "        ROUND(AVG(total)) as total, "
            "        ROUND(AVG(used)) as used, "
            "        ROUND(AVG(free)) as free, "
            "        AVG(usage_percent) as usage_percent "
            "    FROM resource_memory "
            "    WHERE host_ip = $1::inet AND time >= to_timestamp($2) AND time <= to_timestamp($3) "
            "    GROUP BY bucket "
            ") AS gapfilled_data "
            "ORDER BY ts ASC",
            host_ip, start_time, end_time
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

    // Network - 按 interface 分组，使用 time_bucket_gapfill 优化
    if (query_all || need.count("network")) {
        // 使用 time_bucket_gapfill 替代 CROSS JOIN，性能更好
        std::string network_query = 
            "SELECT "
            "    interface, "
            "    EXTRACT(EPOCH FROM bucket)::bigint AS ts, "
            "    COALESCE(ROUND(AVG(rx_bytes)), 0)::bigint AS rx_bytes, "
            "    COALESCE(ROUND(AVG(tx_bytes)), 0)::bigint AS tx_bytes, "
            "    COALESCE(ROUND(AVG(rx_packets)), 0)::bigint AS rx_packets, "
            "    COALESCE(ROUND(AVG(tx_packets)), 0)::bigint AS tx_packets, "
            "    COALESCE(ROUND(AVG(rx_errors)), 0)::bigint AS rx_errors, "
            "    COALESCE(ROUND(AVG(tx_errors)), 0)::bigint AS tx_errors, "
            "    COALESCE(AVG(rx_rate), 0) AS rx_rate, "
            "    COALESCE(AVG(tx_rate), 0) AS tx_rate, "
            "    COALESCE(AVG(rx_drop_rate), 0) AS rx_drop_rate, "
            "    COALESCE(AVG(tx_drop_rate), 0) AS tx_drop_rate, "
            "    COALESCE(MODE() WITHIN GROUP (ORDER BY state), 0)::int AS state "
            "FROM ( "
            "    SELECT "
            "        time_bucket_gapfill(" + bucket_interval + ", time, to_timestamp($2), to_timestamp($3)) AS bucket, "
            "        interface, "
            "        rx_bytes, tx_bytes, rx_packets, tx_packets, "
            "        rx_errors, tx_errors, rx_rate, tx_rate, "
            "        rx_drop_rate, tx_drop_rate, state "
            "    FROM resource_network "
            "    WHERE host_ip = $1::inet "
            "      AND time >= to_timestamp($2) AND time <= to_timestamp($3) "
            ") AS gapfilled "
            "GROUP BY interface, bucket "
            "ORDER BY interface, ts ASC";
        pqxx::result r = tx.exec_params(network_query, host_ip, start_time, end_time);
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
            p.rx_rate     = row[8].as<double>(0);
            p.tx_rate     = row[9].as<double>(0);
            p.rx_drop_rate = row[10].as<double>(0);
            p.tx_drop_rate = row[11].as<double>(0);
            p.state       = row[12].as<int>(0);
            out.network[iface].push_back(std::move(p));
        }
    }

    // Disk - 按 device key 分组，使用 time_bucket_gapfill 优化
    if (query_all || need.count("disk")) {
        std::string disk_query = 
            "SELECT "
            "    device, "
            "    mount_point, "
            "    EXTRACT(EPOCH FROM bucket)::bigint AS ts, "
            "    COALESCE(ROUND(AVG(total)), 0)::bigint AS total, "
            "    COALESCE(ROUND(AVG(used)), 0)::bigint AS used, "
            "    COALESCE(ROUND(AVG(free)), 0)::bigint AS free, "
            "    COALESCE(AVG(usage_percent), 0) AS usage_percent "
            "FROM ( "
            "    SELECT "
            "        time_bucket_gapfill(" + bucket_interval + ", time, to_timestamp($2), to_timestamp($3)) AS bucket, "
            "        device, mount_point, total, used, free, usage_percent "
            "    FROM resource_disk "
            "    WHERE host_ip = $1::inet "
            "      AND time >= to_timestamp($2) AND time <= to_timestamp($3) "
            ") AS gapfilled "
            "GROUP BY device, mount_point, bucket "
            "ORDER BY device, ts ASC";
        pqxx::result r = tx.exec_params(disk_query, host_ip, start_time, end_time);
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

    // GPU - 按 gpu_index 分组，使用 time_bucket_gapfill 优化
    if (query_all || need.count("gpu")) {
        std::string gpu_query = 
            "SELECT "
            "    gpu_index, "
            "    name, "
            "    EXTRACT(EPOCH FROM bucket)::bigint AS ts, "
            "    COALESCE(AVG(compute_usage), 0) AS compute_usage, "
            "    COALESCE(AVG(mem_usage), 0) AS mem_usage, "
            "    COALESCE(ROUND(AVG(mem_used)), 0)::bigint AS mem_used, "
            "    COALESCE(ROUND(AVG(mem_total)), 0)::bigint AS mem_total, "
            "    COALESCE(AVG(temperature), 0) AS temperature, "
            "    COALESCE(AVG(power), 0) AS power, "
            "    COALESCE(ROUND(AVG(free)), 0)::integer AS free "
            "FROM ( "
            "    SELECT "
            "        time_bucket_gapfill(" + bucket_interval + ", time, to_timestamp($2), to_timestamp($3)) AS bucket, "
            "        gpu_index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power, free "
            "    FROM resource_gpu "
            "    WHERE host_ip = $1::inet "
            "      AND time >= to_timestamp($2) AND time <= to_timestamp($3) "
            ") AS gapfilled "
            "GROUP BY gpu_index, name, bucket "
            "ORDER BY gpu_index, ts ASC";
        pqxx::result r = tx.exec_params(gpu_query, host_ip, start_time, end_time);
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
            p.free          = row[9].as<int>(0);
            std::string key = std::string("gpu_") + std::to_string(index);
            out.gpu[key].push_back(std::move(p));
        }
    }

    return out;
}

} // namespace monitor
} // namespace yw


