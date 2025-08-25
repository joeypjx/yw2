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

    // bmc_fan: 遍历 2 个风扇
    for (int i = 0; i < 2; ++i) {
        const auto& f = pkt.fan[i];
        tx.exec_params(
            "INSERT INTO bmc_fan(\"time\", boxid, fanseq, fanmode, fanspeed)"
            " VALUES (now(), $1, $2, $3, $4)",
            static_cast<int>(pkt.boxid),
            static_cast<int>(f.fanseq),
            static_cast<int>(f.fanmode),
            static_cast<long long>(f.fanspeed)
        );
    }

    // bmc_sensor: 遍历 14 个板卡，每块最多 5 个传感器
    for (int bi = 0; bi < 14; ++bi) {
        const auto& b = pkt.board[bi];
        for (int si = 0; si < 5; ++si) {
            const auto& s = b.sensor[si];
            // 以 0 作为空项的简单判别：可按实际协议调整
            if (s.sensorseq == 0 && s.sensortype == 0 && s.sensorvalue_L == 0 && s.sensorvalue_H == 0) {
                continue;
            }
            // 名称与版本：将定长字节数组转为字符串（去除尾部\0）
            auto to_str = [](const std::uint8_t* p, size_t n) -> std::string {
                std::string out; out.reserve(n);
                for (size_t i = 0; i < n; ++i) {
                    if (p[i] == 0) break; out.push_back(static_cast<char>(p[i]));
                }
                return out;
            };

            auto ipmbaddrToSlotId = [](std::uint8_t ipmbaddr) -> std::uint8_t {
                switch (ipmbaddr) {
                    case 0x7c: return 1;   case 0x7a: return 2;   case 0x38: return 3;   case 0x76: return 4;
                    case 0x34: return 5;   case 0x32: return 6;   case 0x70: return 7;   case 0x6e: return 8;
                    case 0x2c: return 9;   case 0x2a: return 10;  case 0x68: return 11;  case 0x26: return 12;
                    case 0x02: return 13;  case 0x04: return 14;  default: return 0;
                }
            };
            auto calculateHostIP = [](int box_id, int slot_id) -> std::string {
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
            };

            const int slot_id = ipmbaddrToSlotId(b.ipmbaddr);
            const std::string host_ip = calculateHostIP(pkt.boxid, slot_id);
            if (host_ip.empty()) continue;

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
    pqxx::result r = tx.exec_params(
        "SELECT EXTRACT(EPOCH FROM \"time\")::bigint AS ts, host_ip, sensorseq, sensortype, sensorname, sensorvalue_L, sensorvalue_H, sensoralmtype"
        " FROM bmc_sensor"
        " WHERE host_ip = $1::inet AND \"time\" >= now() - $2::interval"
        " ORDER BY \"time\" DESC",
        host_ip, duration
    );
    for (const auto& row : r) {
        BMCSensorRow e{};
        e.timestamp      = row[0].as<long long>(0);
        e.host_ip        = row[1].as<std::string>("");
        e.sensorseq      = static_cast<std::uint16_t>(row[2].as<int>(0));
        e.sensortype     = static_cast<std::uint16_t>(row[3].as<int>(0));
        e.sensorname     = row[4].as<std::string>("");
        e.sensorvalue_L  = static_cast<std::uint16_t>(row[5].as<int>(0));
        e.sensorvalue_H  = static_cast<std::uint16_t>(row[6].as<int>(0));
        e.sensoralmtype  = static_cast<std::uint16_t>(row[7].as<int>(0));
        out[e.sensorname].push_back(std::move(e));
    }
    return out;
}

} // namespace bmc
} // namespace yw


