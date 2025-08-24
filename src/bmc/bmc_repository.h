#pragma once

#include <string>
#include <vector>
#include "yw/bmc_model.h"
#include <unordered_map>

namespace yw {
namespace bmc {

class BMCRepository {
public:
    explicit BMCRepository(const std::string& conninfo);

    // 将一次 UdpInfo 报文写入 TimescaleDB（bmc_fan / bmc_sensor）
    void save(const UdpInfo& pkt);

    // 查询指定 host_ip 在最近一段时间内的 bmc_sensor 记录，按 sensorname 分组
    // 返回：key 为 sensorname，value 为该传感器的时序点（降序）
    std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration);

private:
    std::string conninfo_;
};

} // namespace bmc
} // namespace yw


