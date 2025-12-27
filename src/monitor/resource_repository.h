// ============================================================================
// 文件功能描述：
// 资源仓库（ResourceRepository）的头文件，定义资源监控数据持久化的接口。
// 主要功能包括：
// 1. 数据持久化：将节点资源数据保存到PostgreSQL数据库（TimescaleDB）
// 2. 时序数据查询：查询指定节点在指定时间范围内的时序指标数据
// 3. 动态分桶：根据时间范围动态调整bucket大小，保持数据点数量在合理范围内
// 4. 多指标支持：支持CPU、内存、磁盘、网络、GPU等多种资源指标的查询
// 5. 连接池管理：使用PostgreSQL连接池管理数据库连接，提高并发性能
// ============================================================================

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


