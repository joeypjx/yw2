#pragma once

#include <memory>
#include "yw/web.h"

namespace hv { class HttpService; }

namespace yw {
namespace node { class INodeModule; }
namespace monitor { class IMonitorModule; }
namespace bmc { class IBMCModule; }
namespace web {

class WebController : public IWebModule {
public:
    WebController(std::shared_ptr<hv::HttpService> service,
                  std::shared_ptr<node::INodeModule> node_module,
                  std::shared_ptr<monitor::IMonitorModule> monitor_module,
                  std::shared_ptr<bmc::IBMCModule> bmc_module);
    ~WebController();

private:
    void setupRoutes();

private:
    std::shared_ptr<hv::HttpService> service_;
    std::shared_ptr<node::INodeModule> node_module_;
    std::shared_ptr<monitor::IMonitorModule> monitor_module_;
    std::shared_ptr<bmc::IBMCModule> bmc_module_;
};

} // namespace web
} // namespace yw


