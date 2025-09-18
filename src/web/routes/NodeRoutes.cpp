#include "NodeRoutes.h"

#include <nlohmann/json.hpp>
#include "dto/node_dto.h"
#include "mapper/NodeMapper.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerNodeRoutes(hv::HttpService* service,
                        node::INodeModule* node_module,
                        monitor::IMonitorModule* monitor_module) {
    if (!service) return;
    service->GET("/node", [node_module, monitor_module](const HttpContextPtr& ctx) {
        if (!node_module) return 500;
        const auto nodes = node_module->getAllNodes();
        int filter_box_id = -1;
        bool has_filter = false;
        std::string filter_host_ip;
        bool has_host_filter = false;
        auto params = ctx->params();
        if (params.find("box_id") != params.end()) {
            try {
                filter_box_id = std::stoi(params["box_id"]);
                has_filter = true;
            } catch (...) {
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "invalid box_id"},
                    {"data", nlohmann::json::object()}
                };
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
        }
        if (params.find("host_ip") != params.end()) {
            filter_host_ip = params["host_ip"];
            has_host_filter = true;
        }
        nlohmann::json resp_nodes = nlohmann::json::array();
        resp_nodes.get_ref<nlohmann::json::array_t&>().reserve(nodes.size());
        for (const auto& ext : nodes) {
            if (has_filter && ext.box_id != filter_box_id) continue;
            if (has_host_filter && ext.host_ip != filter_host_ip) continue;
            const monitor::Resource* res = nullptr;
            std::optional<monitor::Resource> resHolder;
            if (monitor_module) {
                auto resPtr = monitor_module->getNodeResource(ext.host_ip);
                if (resPtr) { resHolder = *resPtr; res = &(*resHolder); }
            }
            auto v = yw::web::mapper::toNodeView(ext, res);
            resp_nodes.push_back(std::move(v));
        }

        nlohmann::json resp;
        if (has_host_filter) {
            if (!resp_nodes.empty()) {
                resp = {
                    {"api_version", 1},
                    {"data", resp_nodes[0]},
                    {"status", "success"},
                };
            } else {
                resp = {
                    {"api_version", 1},
                    {"data", json::object()},
                    {"status", "success"},
                };
            }
        } else {
            resp = {
                {"api_version", 1},
                {"data", {
                    {"nodes", resp_nodes}
                }},
                {"status", "success"},
            };
        }

        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });
}

} // namespace routes
} // namespace web
} // namespace yw


