// ============================================================================
// 文件功能描述：
// BMC仓库（BMCRepository）的头文件，定义BMC传感器数据持久化的接口。
// 主要功能包括：
// 1. 数据持久化：将UDP组播接收到的BMC数据保存到PostgreSQL数据库（TimescaleDB）
// 2. 时序数据查询：查询指定节点在指定时间范围内的BMC传感器时序数据
// 3. 最新数据查询：获取指定节点的最新BMC传感器数据（每个传感器只返回最新一条记录）
// 4. 连接池管理：使用PostgreSQL连接池管理数据库连接，提高并发性能
// ============================================================================

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


