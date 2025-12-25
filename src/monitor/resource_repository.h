#pragma once

#include <string>
#include <memory>
#include "monitor/monitor_model.h"

// 前向声明
namespace yw {
namespace utils {
    class PostgreSQLConnectionPool;
    class ConnectionGuard;
}
}

namespace yw {
namespace monitor {

class ResourceRepository {
public:
    /**
     * @brief 构造函数
     * @param conninfo 数据库连接信息
     * @param minConnections 连接池最小连接数（默认2）
     * @param maxConnections 连接池最大连接数（默认10）
     */
    explicit ResourceRepository(const std::string& conninfo,
                               size_t minConnections = 2,
                               size_t maxConnections = 10);
    
    ~ResourceRepository();

    // 将一次 /resource 的 data 写入 TimescaleDB（多表，单事务）
    void save(const Resource& data);

    // 查询某个节点在最近一段时间内的指定资源类型数据点序列
    // duration 例如 "60 seconds" / "1 minute" / "30 seconds"
    // kinds 可包含："cpu","memory","network","disk","gpu"；为空表示全部
    MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                     const std::string& duration,
                                     const std::vector<std::string>& kinds);

    // 查询某个节点在指定起止时间范围内的指定资源类型数据点序列
    // start_time, end_time 为秒级时间戳
    // kinds 可包含："cpu","memory","network","disk","gpu"；为空表示全部
    MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                     std::int64_t start_time,
                                     std::int64_t end_time,
                                     const std::vector<std::string>& kinds);

private:
    std::unique_ptr<yw::utils::PostgreSQLConnectionPool> connectionPool_;
};

} // namespace monitor
} // namespace yw


