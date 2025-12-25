#pragma once

#include <string>
#include <vector>
#include <memory>
#include "bmc/bmc_model.h"
#include <unordered_map>

// 前向声明
namespace yw {
namespace utils {
    class PostgreSQLConnectionPool;
    class ConnectionGuard;
}
}

namespace yw {
namespace bmc {

class BMCRepository {
public:
    /**
     * @brief 构造函数
     * @param conninfo 数据库连接信息
     * @param minConnections 连接池最小连接数（默认2）
     * @param maxConnections 连接池最大连接数（默认10）
     */
    explicit BMCRepository(const std::string& conninfo,
                          size_t minConnections = 2,
                          size_t maxConnections = 10);
    
    ~BMCRepository();

    // 将一次 UdpInfo 报文写入 TimescaleDB（bmc_fan / bmc_sensor）
    void save(const UdpInfo& pkt);

    // 查询指定 host_ip 在最近一段时间内的 bmc_sensor 记录，按 sensorname 分组
    // 返回：key 为 sensorname，value 为该传感器的时序点（降序）
    std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration);

    // 查询指定 host_ip 的最新 bmc_sensor 记录，按 sensorname 分组
    // 返回：key 为 sensorname，value 为该传感器的最新记录
    std::unordered_map<std::string, BMCSensorRow> getLatestBMCSensor(
        const std::string& host_ip);

private:
    std::unique_ptr<yw::utils::PostgreSQLConnectionPool> connectionPool_;
};

} // namespace bmc
} // namespace yw


