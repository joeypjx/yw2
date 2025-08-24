#pragma once

#include <memory>
#include <vector>
#include <string>
#include "yw/monitor_model.h"

// 前向声明
namespace hv { class HttpService; }
namespace yw { namespace node { class INodeModule; } }

namespace yw {
namespace monitor {

struct Resource; // 前向声明，避免在公共头包含私有实现头

class IMonitorModule {
public:
    virtual ~IMonitorModule() = default;
    // 根据节点IP返回最近一次上报的资源快照；未命中返回 nullptr
    virtual std::shared_ptr<Resource> getNodeResource(const std::string& host_ip) const = 0;

    // 查询一段时间窗口内的指标序列（kinds: cpu/memory/network/disk/gpu 空表示全部）
    // duration 允许简写：1h/5m/10s（仅单一单位）
    virtual MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                             const std::string& duration,
                                             const std::vector<std::string>& kinds) const = 0;
};

class MonitorFactory {
public:
    static std::shared_ptr<IMonitorModule> getMonitorModule(
        std::shared_ptr<hv::HttpService> service,
        std::shared_ptr<node::INodeModule> node_module);
};

} // namespace monitor
} // namespace yw


