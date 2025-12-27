// ============================================================================
// 文件功能描述：
// BMC仓库（BMCRepository）的实现文件，负责BMC传感器数据的数据库持久化和查询。
// 主要功能包括：
// 1. 数据持久化：将UDP组播接收到的BMC数据（风扇、传感器、板卡信息）保存到PostgreSQL数据库
// 2. 时序数据存储：使用TimescaleDB时序表存储BMC传感器数据，支持高效的时间序列查询
// 3. 传感器数据查询：查询指定节点在指定时间范围内的BMC传感器时序数据（每10秒聚合）
// 4. 最新数据查询：获取指定节点的最新BMC传感器数据（每个传感器只返回最新一条记录）
// 5. IP地址计算：使用IPAddressUtils工具类根据机箱号和槽位号计算节点IP地址
// 6. 槽位映射：处理负载槽映射关系（协议中负载槽顺序与物理槽位号的对应关系）
// 7. 连接池管理：使用PostgreSQL连接池管理数据库连接，提高并发性能
// ============================================================================

#include "bmc_repository.h"
#include "utils/postgresql_connection_pool.h"
#include "utils/ip_address_utils.h"
#include <pqxx/pqxx>
#include <string>

namespace yw {
namespace bmc {

// BMC仓库构造函数
// conninfo: PostgreSQL连接字符串
// minConnections: 连接池最小连接数
// maxConnections: 连接池最大连接数
BMCRepository::BMCRepository(const std::string& conninfo,
                             size_t minConnections,
                             size_t maxConnections)
    : connectionPool_(std::make_unique<yw::utils::PostgreSQLConnectionPool>(conninfo, minConnections, maxConnections)) {
}

// BMC仓库析构函数
BMCRepository::~BMCRepository() = default;

// 保存BMC UDP数据包到数据库
// pkt: UDP组播接收到的BMC信息（包含风扇、传感器、板卡信息等）
// 将数据插入到对应的时序表中（bmc_fan, bmc_sensor等）
void BMCRepository::save(const UdpInfo& pkt) {
    // 从连接池获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    pqxx::work tx{*conn};

    // bmc_fan: 遍历 6 个风扇（根据协议更新）
    for (int i = 0; i < 6; ++i) {
        const auto& f = pkt.fan[i];
        // 跳过无效风扇（序号为0xFF表示无此风扇）
        if (f.fanseq == 0xFF) continue;
        tx.exec_params(
            "INSERT INTO bmc_fan(\"time\", boxid, fanseq, fanmode, fanspeed)"
            " VALUES (now(), $1, $2, $3, $4)",
            static_cast<int>(pkt.boxid),
            static_cast<int>(f.fanseq),
            static_cast<int>(f.fanmode),
            static_cast<long long>(f.fanspeed)
        );
    }

    // 名称与版本：将定长字节数组转为字符串（去除尾部\0）
    auto to_str = [](const std::uint8_t* p, size_t n) -> std::string {
        std::string out; out.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            if (p[i] == 0) break; out.push_back(static_cast<char>(p[i]));
        }
        return out;
    };

    // 使用 IPAddressUtils 工具类计算 IP 地址
    // 注意：slot_id == 0（电源模块）时，IPAddressUtils 会返回空字符串，符合预期

    // 负载槽映射：协议中负载槽顺序是：槽1、槽2、槽3、槽4、槽6、槽7、槽9、槽10、槽11、槽12
    static const int slot_mapping[] = {1, 2, 3, 4, 6, 7, 9, 10, 11, 12};

    // bmc_sensor: 处理 2 个电源模块
    for (int pi = 0; pi < 2; ++pi) {
        const auto& dyboard = pkt.dyboard[pi];
        // 电源模块不映射到slot_id，不计算host_ip，跳过传感器保存
        // 如果需要保存电源模块的传感器数据，可以在这里添加逻辑
    }

    // bmc_sensor: 处理 10 个负载槽，每块最多 8 个传感器
    for (int si = 0; si < 10; ++si) {
        const auto& board = pkt.board[si];
        // 检查负载槽在位信息
        if (board.prst == 0) {
            // 负载槽不在位，跳过
            continue;
        }
        
        // 获取对应的槽位ID
        int slot_id = slot_mapping[si];
        const std::string host_ip = yw::utils::IPAddressUtils::calculateHostIP(pkt.boxid, slot_id);
        if (host_ip.empty()) continue;
        
        // 根据传感器数量处理（最多8个）
        int sensor_count = (board.sensornum > 8) ? 8 : board.sensornum;
        for (int sensor_idx = 0; sensor_idx < sensor_count; ++sensor_idx) {
            const auto& s = board.sensor[sensor_idx];
            // 跳过无效传感器（序号为0xFF表示无此传感器）
            if (s.sensorseq == 0xFF) continue;
            // 以 0 作为空项的简单判别
            if (s.sensorseq == 0 && s.sensortype == 0 && s.sensorvalue_L == 0 && s.sensorvalue_H == 0) {
                continue;
            }

            tx.exec_params(
                "INSERT INTO bmc_sensor(\"time\", host_ip, sensorseq, sensortype, sensorname, sensorvalue_L, sensorvalue_H, sensoralmtype)"
                " VALUES (now(), $1::inet, $2, $3, $4, $5, $6, $7)",
                host_ip,
                static_cast<int>(s.sensorseq),
                static_cast<int>(s.sensortype),
                to_str(s.sensorname, 6),
                static_cast<int>(s.sensorvalue_L),
                static_cast<int>(s.sensorvalue_H),
                static_cast<int>(s.sensoralmtype)
            );
        }
    }

    tx.commit();
}

// 查询指定节点的BMC传感器时序数据
// host_ip: 节点IP地址
// duration: 时间范围（PostgreSQL interval格式，如"5 minutes"）
// 返回: BMC传感器数据映射（传感器名称->传感器时序数据列表）
std::unordered_map<std::string, std::vector<BMCSensorRow>> BMCRepository::queryBMCSensor(
    const std::string& host_ip,
    const std::string& duration) {
    std::unordered_map<std::string, std::vector<BMCSensorRow>> out;
    
    // 从连接池获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    pqxx::read_transaction tx{*conn};

    const char* bmc_query = R"SQL(
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
sensor_dims AS (
  SELECT DISTINCT sensorname, sensorseq, sensortype FROM bmc_sensor
  WHERE host_ip = $1::inet
    AND "time" >= now() - $2::interval AND "time" <= now()
)
SELECT
    dims.sensorname,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    $1::inet AS host_ip,
    dims.sensorseq,
    dims.sensortype,
    COALESCE(ROUND(AVG(metrics.sensorvalue_L)), 0)::smallint AS sensorvalue_L,
    COALESCE(ROUND(AVG(metrics.sensorvalue_H)), 0)::smallint AS sensorvalue_H,
    COALESCE(ROUND(AVG(metrics.sensoralmtype)), 0)::smallint AS sensoralmtype
FROM
    sensor_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    bmc_sensor AS metrics
ON
    metrics.sensorname = dims.sensorname
    AND metrics.sensorseq = dims.sensorseq
    AND metrics.sensortype = dims.sensortype
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics."time") = buckets.bucket
    AND metrics."time" >= now() - $2::interval AND metrics."time" <= now()
GROUP BY
    dims.sensorname, dims.sensorseq, dims.sensortype, buckets.bucket
ORDER BY
    dims.sensorname, ts ASC
)SQL";
    pqxx::result r = tx.exec_params(bmc_query, host_ip, duration);

    for (const auto& row : r) {
        BMCSensorRow e{};
        const std::string sensorname = row[0].as<std::string>("");
        e.timestamp      = row[1].as<long long>(0);
        e.host_ip        = row[2].as<std::string>("");
        e.sensorseq      = static_cast<std::uint16_t>(row[3].as<int>(0));
        e.sensortype     = static_cast<std::uint16_t>(row[4].as<int>(0));
        e.sensorname     = sensorname;
        e.sensorvalue_L  = static_cast<std::uint16_t>(row[5].as<int>(0));
        e.sensorvalue_H  = static_cast<std::uint16_t>(row[6].as<int>(0));
        e.sensor_value   = static_cast<std::double_t>(e.sensorvalue_H) + static_cast<std::double_t>(e.sensorvalue_L) * 0.01;
        e.sensoralmtype  = static_cast<std::uint16_t>(row[7].as<int>(0));
        out[sensorname].push_back(std::move(e));
    }

    for (auto& [sensorname, points] : out) {
        points.erase(points.begin());
        points.erase(points.end() - 1);
    }

    return out;
}

// 获取指定节点的最新BMC传感器数据（每个传感器只返回最新一条记录）
// host_ip: 节点IP地址
// 返回: BMC传感器数据映射（传感器名称->最新传感器数据）
// 注意：只查询最近5分钟的数据，避免扫描整个表
std::unordered_map<std::string, BMCSensorRow> BMCRepository::getLatestBMCSensor(
    const std::string& host_ip) {
    std::unordered_map<std::string, BMCSensorRow> out;
    
    // 从连接池获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    pqxx::read_transaction tx{*conn};

    const char* bmc_query = R"SQL(
SELECT DISTINCT ON (sensorname)
    sensorname,
    EXTRACT(EPOCH FROM "time")::bigint AS timestamp,
    host_ip::text,
    sensorseq,
    sensortype,
    sensorvalue_L,
    sensorvalue_H,
    sensoralmtype
FROM bmc_sensor
WHERE host_ip = $1::inet
  AND "time" >= NOW() - INTERVAL '5 minutes'
ORDER BY sensorname, "time" DESC
)SQL";
    pqxx::result r = tx.exec_params(bmc_query, host_ip);

    for (const auto& row : r) {
        BMCSensorRow e{};
        const std::string sensorname = row[0].as<std::string>("");
        e.timestamp      = row[1].as<long long>(0);
        e.host_ip        = row[2].as<std::string>("");
        e.sensorseq      = static_cast<std::uint16_t>(row[3].as<int>(0));
        e.sensortype     = static_cast<std::uint16_t>(row[4].as<int>(0));
        e.sensorname     = sensorname;
        e.sensorvalue_L  = static_cast<std::uint16_t>(row[5].as<int>(0));
        e.sensorvalue_H  = static_cast<std::uint16_t>(row[6].as<int>(0));
        e.sensor_value   = static_cast<std::double_t>(e.sensorvalue_H) + static_cast<std::double_t>(e.sensorvalue_L) * 0.01;
        e.sensoralmtype  = static_cast<std::uint16_t>(row[7].as<int>(0));
        out[sensorname] = std::move(e);
    }

    return out;
}

} // namespace bmc
} // namespace yw


