#pragma once

#include <memory>
#include <vector>

// 前向声明
namespace hv { class HttpService; }
namespace yw { namespace node { class INodeModule; } }

namespace yw {
namespace monitor {

class IMonitorModule {
public:
    virtual ~IMonitorModule() = default;
};

class MonitorFactory {
public:
    static std::shared_ptr<IMonitorModule> getMonitorModule(
        std::shared_ptr<hv::HttpService> service,
        std::shared_ptr<node::INodeModule> node_module);
};

} // namespace monitor
} // namespace yw


