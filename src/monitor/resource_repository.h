#pragma once

#include <string>
#include "yw/monitor_model.h"

namespace yw {
namespace monitor {

class ResourceRepository {
public:
    explicit ResourceRepository(const std::string& conninfo);

    // 将一次 /resource 的 data 写入 TimescaleDB（多表，单事务）
    void save(const Resource& data);

    // 查询某个节点在最近一段时间内的指定资源类型数据点序列
    // duration 例如 "60 seconds" / "1 minute" / "30 seconds"
    // kinds 可包含："cpu","memory","network","disk","gpu"；为空表示全部
    MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                     const std::string& duration,
                                     const std::vector<std::string>& kinds);

private:
    std::string conninfo_;
};

} // namespace monitor
} // namespace yw


