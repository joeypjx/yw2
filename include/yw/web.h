#pragma once

#include <memory>

namespace hv { class HttpService; class HttpServer; }

namespace yw {
namespace node { class INodeModule; }
namespace monitor { class IMonitorModule; }
namespace bmc { class IBMCModule; }
namespace alert { class IAlertModule; }
namespace web {

class IWebModule {
public:
    virtual ~IWebModule() = default;
    virtual void setupRoutes() = 0;
};

class WebFactory {
public:
    static std::shared_ptr<IWebModule> getWebModule(std::shared_ptr<hv::HttpServer> server,
                                                    std::shared_ptr<hv::HttpService> service,
                                                    std::shared_ptr<node::INodeModule> node_module,
                                                    std::shared_ptr<monitor::IMonitorModule> monitor_module,
                                                    std::shared_ptr<bmc::IBMCModule> bmc_module,
                                                    std::shared_ptr<alert::IAlertModule> alert_module);
};

} // namespace web
} // namespace yw


