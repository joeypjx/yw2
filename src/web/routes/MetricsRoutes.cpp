#include "MetricsRoutes.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <spdlog/spdlog.h>
#include "dto/node_dto.h"
#include "mapper/NodeMapper.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerMetricsRoutes(hv::HttpService* service,
                           node::INodeModule* node_module,
                           monitor::IMonitorModule* monitor_module,
                           bmc::IBMCModule* bmc_module) {
    if (!service) return;

    // /node/metrics
    service->GET("/node/metrics", [node_module, monitor_module](const HttpContextPtr& ctx) {
        if (!node_module) {
            nlohmann::json resp = {{"api_version",1},{"status","error"},{"message","node module unavailable"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        const auto now_ms = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::system_clock::now()
        ).time_since_epoch().count();

        auto nodes = node_module->getAllNodes();
        std::vector<NodeMetrics> result;
        result.reserve(nodes.size());
        for (const auto& nx : nodes) {
            const monitor::Resource* res = nullptr;
            std::optional<monitor::Resource> resHolder;
            if (monitor_module) {
                auto resPtr = monitor_module->getNodeResource(nx.host_ip);
                if (resPtr) { resHolder = *resPtr; res = &(*resHolder); }
            }
            result.push_back(yw::web::mapper::toNodeMetrics(nx, res, now_ms));
        }

        json resp = {
            {"api_version", 1},
            {"data", {
                {"nodes_metrics", result}
            }},
            {"status", "success"},
        };
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // /node/historical-metrics
    service->GET("/node/historical-metrics", [node_module, monitor_module, bmc_module](const HttpContextPtr& ctx) {
        if (!monitor_module) {
            nlohmann::json resp = {{"api_version",1},{"status","error"},{"message","monitor module unavailable"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }

        std::string ip;
        std::string duration = "1m";
        std::vector<std::string> kinds;

        auto params = ctx->params();
        if (params.find("host_ip") != params.end()) ip = params["host_ip"];
        if (params.find("time_range") != params.end()) duration = params["time_range"];
        if (params.find("metrics") != params.end()) {
            const std::string& ks = params["metrics"];
            std::string item;
            for (size_t i = 0, n = ks.size(); i <= n; ++i) {
                if (i == n || ks[i] == ',') { if (!item.empty()) kinds.push_back(item); item.clear(); }
                else item.push_back(ks[i]);
            }
        }

        if (ip.empty()) {
            nlohmann::json resp = {{"api_version",1},{"status","error"},{"message","missing host_ip"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }

        try {
            auto series = monitor_module->queryMetricsSeries(ip, duration, kinds);
            monitor::ResourceWindow win;
            win.host_ip = ip;
            win.metrics = std::move(series);
            win.time_range = duration;
            if (node_module) {
                auto nodeOpt = node_module->getNodeByIP(ip);
                if (nodeOpt) {
                    win.box_id = nodeOpt->box_id;
                    win.cpu_id = nodeOpt->cpu_id;
                    win.slot_id = nodeOpt->slot_id;
                }
            }

            nlohmann::json bmc_json;
            nlohmann::json* bmc_ptr = nullptr;
            if (bmc_module) {
                auto grouped = bmc_module->queryBMCSensor(ip, duration);
                bmc_json = grouped;
                bmc_ptr = &bmc_json;
            }

            auto view = yw::web::mapper::toHistoricalMetricsView(win, bmc_ptr);

            nlohmann::json resp = {
                {"api_version", 1},
                {"data", {
                    {"historical_metrics", view}
                }},
                {"status", "success"},
            };

            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            spdlog::error("query resource failed: {}", e.what());
            nlohmann::json resp = {{"api_version",1},{"status","error"},{"message","internal error"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw


