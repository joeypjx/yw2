#pragma once

#include <memory>
#include "monitor/monitor.h"
#include "resource_repository.h"
#include "monitor_cache.h"

// 前向声明，避免在头文件中引入平台相关头
namespace hv {
    class HttpServer;
    struct HttpService;
}

namespace yw {
namespace utils {
    class MulticastScanner;
}
}

namespace yw {
namespace monitor {

/**
 * @brief 监控管理器实现类
 * 
 * 负责管理节点资源监控功能，包括资源快照缓存、数据库持久化和 HTTP 路由设置。
 */
class MonitorManager : public IMonitorModule {
public:
    explicit MonitorManager(std::shared_ptr<hv::HttpService> service);
    ~MonitorManager();

    // ========== IMonitorModule 接口实现 ==========

    std::shared_ptr<Resource> getNodeResource(const std::string& host_ip) const override;
    
    MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                     const std::string& duration,
                                     const std::vector<std::string>& kinds) const override;
    
    ExportData exportNodeData(const std::string& host_ip,
                              std::int64_t start_time,
                              std::int64_t end_time,
                              const std::vector<std::string>& types) const override;

private:
    void setupRoutes();

private:
    std::shared_ptr<hv::HttpService> service_;              // HTTP 服务实例
    std::unique_ptr<yw::utils::MulticastScanner> scanner_;  // 通用组播扫描器
    std::unique_ptr<ResourceRepository> repository_;        // TimescaleDB 写入
    std::unique_ptr<MonitorCache> monitor_cache_;           // 资源最新快照缓存
};

} // namespace monitor
} // namespace yw


