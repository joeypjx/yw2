#pragma once

#include <memory>
#include "yw/monitor.h"
#include "resource_repository.h"

// 前向声明，避免在头文件中引入平台相关头
namespace hv {
    class HttpServer;
    class HttpService;
}

namespace yw { namespace utils { class MulticastScanner; } }
namespace yw { namespace node { class INodeModule; } }

namespace yw {
namespace monitor {

class MonitorManager : public IMonitorModule {
public:
    MonitorManager(std::shared_ptr<hv::HttpService> service,
                   std::shared_ptr<node::INodeModule> node_module);
    ~MonitorManager();

private:
    void setupRoutes();

private:
    std::shared_ptr<hv::HttpService> service_;
    std::unique_ptr<yw::utils::MulticastScanner> scanner_; // 通用组播扫描器
    std::unique_ptr<ResourceRepository> repository_;        // TimescaleDB 写入
    std::shared_ptr<node::INodeModule> node_module_;        // 共享的节点模块
};

} // namespace monitor
} // namespace yw


