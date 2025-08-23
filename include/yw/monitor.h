#pragma once

#include <memory>
#include <vector>

// 前向声明
namespace hv { class HttpService; }

namespace yw {
namespace monitor {

class IMonitorModule {
public:
    virtual ~IMonitorModule() = default;
};

class MonitorFactory {
public:
    static std::shared_ptr<IMonitorModule> getMonitorModule(std::shared_ptr<hv::HttpService> service);
};

} // namespace monitor
} // namespace yw


