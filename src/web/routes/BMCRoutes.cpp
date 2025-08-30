#include "BMCRoutes.h"

#include <nlohmann/json.hpp>

namespace yw {
namespace web {
namespace routes {

void registerBMCRoutes(hv::HttpService* service,
                       bmc::IBMCModule* bmc_module) {
    if (!service) return;
    service->GET("/box/bmc", [bmc_module](const HttpContextPtr& ctx) {
        if (!bmc_module) return 500;
        std::string box_id_param = ctx->param("box_id");
        if (box_id_param.empty()) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "box_id parameter is required"}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        }
        std::string duration = ctx->param("duration");
        if (duration.empty()) duration = "5m";
        try {
            int box_id = std::stoi(box_id_param);
            auto info_opt = bmc_module->getBoxBMC(box_id);
            if (!info_opt.has_value()) {
                nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "Box not found or no BMC data available"}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            }
            const auto& info = info_opt.value();
            nlohmann::json j;
            j["box_id"] = info.boxid;
            j["fan_0_speed"] = info.fan[0].fanspeed;
            j["fan_1_speed"] = info.fan[1].fanspeed;
            if (bmc_module) {
                auto grouped = bmc_module->queryBMCSensor("192.168." + std::to_string(info.boxid *2 + 1) + ".180", duration);
                if (grouped.size() == 0) {
                    grouped = bmc_module->queryBMCSensor("192.168." + std::to_string(info.boxid *2 + 1) + ".181", duration);
                }
                nlohmann::json bmc_json = grouped;
                j["sensor"] = std::move(bmc_json);
            }
            nlohmann::json resp = {{"api_version", 1},{"data", j},{"status", "success"}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        } catch (...) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "Invalid box_id parameter"}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw


