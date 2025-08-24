#pragma once

#include <memory>

namespace hv { class HttpService; }

namespace yw {
namespace node { class INodeModule; }
namespace monitor { class IMonitorModule; }
namespace bmc { class IBMCModule; }
namespace web {

class IWebModule {
public:
    virtual ~IWebModule() = default;
};

class WebFactory {
public:
    static std::shared_ptr<IWebModule> getWebModule(std::shared_ptr<hv::HttpService> service,
                                                    std::shared_ptr<node::INodeModule> node_module,
                                                    std::shared_ptr<monitor::IMonitorModule> monitor_module,
                                                    std::shared_ptr<bmc::IBMCModule> bmc_module);
};

} // namespace web
} // namespace yw


