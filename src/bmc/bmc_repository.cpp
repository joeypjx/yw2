#include "bmc_repository.h"
#include <pqxx/pqxx>
#include <string>

namespace yw {
namespace bmc {

namespace {
static std::uint8_t ipmbaddrToSlotId(std::uint8_t ipmbaddr) {
    switch (ipmbaddr) {
        case 0x7c: return 1;   case 0x7a: return 2;   case 0x38: return 3;   case 0x76: return 4;
        case 0x34: return 5;   case 0x32: return 6;   case 0x70: return 7;   case 0x6e: return 8;
        case 0x2c: return 9;   case 0x2a: return 10;  case 0x68: return 11;  case 0x26: return 12;
        case 0x02: return 13;  case 0x04: return 14;  default: return 0;
    }
}
static std::string calculateHostIP(int box_id, int slot_id) {
    int network_id; int host_id;
    if (slot_id >= 1 && slot_id <= 7) {
        network_id = box_id * 2;
        switch (slot_id) {
            case 1: host_id = 5; break; case 2: host_id = 37; break; case 3: host_id = 69; break;
            case 4: host_id = 101; break; case 5: host_id = 133; break; case 6: host_id = 170; break; case 7: host_id = 180; break;
            default: host_id = 5; break;
        }
    } else if (slot_id >= 8 && slot_id <= 12) {
        network_id = box_id * 2 + 1;
        switch (slot_id) {
            case 8: host_id = 5; break; case 9: host_id = 37; break; case 10: host_id = 69; break; case 11: host_id = 101; break; case 12: host_id = 133; case 13: host_id = 181; break; case 14: host_id = 182; break;
            default: host_id = 5; break;
        }
    } else { return std::string(); }
    return std::string("192.168.") + std::to_string(network_id) + "." + std::to_string(host_id);
}
} // namespace

BMCRepository::BMCRepository(const std::string& conninfo)
    : conninfo_(conninfo) {}

void BMCRepository::save(const UdpInfo& pkt) {
    pqxx::connection c(conninfo_);
    pqxx::work tx{c};

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

    auto calculateHostIP = [](int box_id, int slot_id) -> std::string {
        if (slot_id == 0) return std::string(); // 电源模块不计算host_ip
        int network_id; int host_id;
        if (slot_id >= 1 && slot_id <= 7) {
            network_id = box_id * 2;
            switch (slot_id) {
                case 1: host_id = 5; break; 
                case 2: host_id = 37; break; 
                case 3: host_id = 69; break;
                case 4: host_id = 101; break;
                case 5: host_id = 133; break;
                case 6: host_id = 170; break;
                case 7: host_id = 180; break;
                default: return std::string();
            }
        } else if (slot_id >= 8 && slot_id <= 12) {
            network_id = box_id * 2 + 1;
            switch (slot_id) {
                case 8: host_id = 5; break;
                case 9: host_id = 37; break;
                case 10: host_id = 69; break;
                case 11: host_id = 101; break;
                case 12: host_id = 133; break;
                default: return std::string();
            }
        } else { 
            return std::string(); 
        }
        return std::string("192.168.") + std::to_string(network_id) + "." + std::to_string(host_id);
    };

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
        const std::string host_ip = calculateHostIP(pkt.boxid, slot_id);
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

std::unordered_map<std::string, std::vector<BMCSensorRow>> BMCRepository::queryBMCSensor(
    const std::string& host_ip,
    const std::string& duration) {
    std::unordered_map<std::string, std::vector<BMCSensorRow>> out;
    pqxx::connection c(conninfo_);
    pqxx::read_transaction tx{c};

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

std::unordered_map<std::string, BMCSensorRow> BMCRepository::getLatestBMCSensor(
    const std::string& host_ip) {
    std::unordered_map<std::string, BMCSensorRow> out;
    pqxx::connection c(conninfo_);
    pqxx::read_transaction tx{c};

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


