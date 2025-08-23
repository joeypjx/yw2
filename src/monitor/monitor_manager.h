#pragma once

#include <memory>
#include "yw/monitor.h"

// 前向声明，避免在头文件中引入平台相关头
namespace hv {
    class HttpServer;
    class HttpService;
}

namespace yw { namespace utils { class MulticastScanner; } }

namespace yw {
namespace monitor {

class MonitorManager : public IMonitorModule {
public:
    explicit MonitorManager(std::shared_ptr<hv::HttpService> service);
    ~MonitorManager();

private:
    void setupRoutes();

private:
    std::shared_ptr<hv::HttpService> service_;
    std::unique_ptr<yw::utils::MulticastScanner> scanner_; // 通用组播扫描器
};

} // namespace monitor
} // namespace yw


