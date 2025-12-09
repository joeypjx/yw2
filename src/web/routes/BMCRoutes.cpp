#include "BMCRoutes.h"
// #include "yw/ipmi.h"  // 暂时注释掉，因为 IPMI 模块暂时不编译

#include <nlohmann/json.hpp>

namespace yw {
namespace web {
namespace routes {

void registerBMCRoutes(hv::HttpService* service,
                       bmc::IBMCModule* bmc_module,
                       controller::IControllerModule* controller_module) {
    if (!service) return;
    service->GET("/box/bmc", [bmc_module](const HttpContextPtr& ctx) {
        if (!bmc_module) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "bmc module unavailable"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        std::string box_id_param = ctx->param("box_id");
        if (box_id_param.empty()) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "box_id parameter is required"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        std::string duration = ctx->param("duration");
        if (duration.empty()) duration = "5m";
        try {
            int box_id = std::stoi(box_id_param);
            auto info_opt = bmc_module->getBoxBMC(box_id);
            if (!info_opt.has_value()) {
                nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "Box not found or no BMC data available"},{"data", nlohmann::json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_NOT_FOUND);
                return ctx->send(resp.dump(2));
            }
            const auto& info = info_opt.value();
            nlohmann::json j;
            j["box_id"] = info.boxid;
            j["fan_0_speed"] = info.fan[0].fanspeed;
            j["fan_1_speed"] = info.fan[1].fanspeed;
            if (bmc_module) {
                auto grouped = bmc_module->queryBMCSensor("192.168." + std::to_string(info.boxid *2) + ".180", duration);
                if (grouped.size() == 0) {
                    grouped = bmc_module->queryBMCSensor("192.168." + std::to_string(info.boxid *2) + ".181", duration);
                }
                nlohmann::json bmc_json = grouped;
                j["sensor"] = std::move(bmc_json);
            }
            nlohmann::json resp = {{"api_version", 1},{"data", j},{"status", "success"}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        } catch (...) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "Invalid box_id parameter"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
    });

    // reset box board
    service->POST("/box/reset_board", [controller_module](const HttpContextPtr& ctx) {
        // 解析 JSON 请求体: {"box_id": <int>, "slot_id": array of int}
        nlohmann::json req;
        try {
            req = nlohmann::json::parse(ctx->body());
        } catch (...) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "invalid json body"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        if (!req.contains("box_id") || !req["box_id"].is_number_integer() || !req.contains("slot_id") || !req["slot_id"].is_array()) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "box_id and slot_id are required integers"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        int box_id = req["box_id"].get<int>();
        std::vector<int> slot_id = req["slot_id"].get<std::vector<int>>();
        for (int slot : slot_id) {
            if (slot < 1 || slot > 12) {
                nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "slot_id is out of range"},{"data", nlohmann::json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
        }
        // 调用 controller 模块的 resetBoard 方法
        // box ip is 192.168. box_id * 2 .180
        std::string box_ip = std::string("192.168.") + std::to_string(box_id * 2) + ".180";
        auto response = controller_module->resetBoard(box_ip, slot_id);
        if (response.result == controller::IControllerModule::OperationResult::SUCCESS) {
            nlohmann::json resp = {{"api_version", 1},{"status", "success"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_OK);
            return ctx->send(resp.dump(2));
        } else {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", response.message},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // power on box board
    service->POST("/box/poweron_board", [controller_module](const HttpContextPtr& ctx) {
        // 解析 JSON 请求体: {"box_id": <int>, "slot_id": array of int}
        nlohmann::json req;
        try {
            req = nlohmann::json::parse(ctx->body());
        } catch (...) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "invalid json body"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        if (!req.contains("box_id") || !req["box_id"].is_number_integer() || !req.contains("slot_id") || !req["slot_id"].is_array()) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "box_id and slot_id are required integers"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        int box_id = req["box_id"].get<int>();
        std::vector<int> slot_id = req["slot_id"].get<std::vector<int>>();
        for (int slot : slot_id) {
            if (slot < 1 || slot > 12) {
                nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "slot_id is out of range"},{"data", nlohmann::json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
        }
        // 调用 controller 模块的 powerOnChassisBoards 方法
        std::string box_ip = std::string("192.168.") + std::to_string(box_id * 2) + ".180";
        auto response = controller_module->powerOnChassisBoards(box_ip, slot_id);
        if (response.result == controller::IControllerModule::OperationResult::SUCCESS || 
            response.result == controller::IControllerModule::OperationResult::PARTIAL_SUCCESS) {
            nlohmann::json resp = {{"api_version", 1},{"status", "success"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_OK);
            return ctx->send(resp.dump(2));
        } else {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", response.message},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // power off box board
    service->POST("/box/poweroff_board", [controller_module](const HttpContextPtr& ctx) {
        // 解析 JSON 请求体: {"box_id": <int>, "slot_id": array of int}
        nlohmann::json req;
        try {
            req = nlohmann::json::parse(ctx->body());
        } catch (...) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "invalid json body"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        if (!req.contains("box_id") || !req["box_id"].is_number_integer() || !req.contains("slot_id") || !req["slot_id"].is_array()) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "box_id and slot_id are required integers"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        int box_id = req["box_id"].get<int>();
        std::vector<int> slot_id = req["slot_id"].get<std::vector<int>>();
        for (int slot : slot_id) {
            if (slot < 1 || slot > 12) {
                nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "slot_id is out of range"},{"data", nlohmann::json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
        }
        // 调用 controller 模块的 powerOffChassisBoards 方法
        std::string box_ip = std::string("192.168.") + std::to_string(box_id * 2) + ".180";
        auto response = controller_module->powerOffChassisBoards(box_ip, slot_id);
        if (response.result == controller::IControllerModule::OperationResult::SUCCESS || 
            response.result == controller::IControllerModule::OperationResult::PARTIAL_SUCCESS) {
            nlohmann::json resp = {{"api_version", 1},{"status", "success"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_OK);
            return ctx->send(resp.dump(2));
        } else {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", response.message},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // 暂时注释掉 fan_speed 相关功能，因为 IPMI 模块暂时不编译
    /*
    service->POST("/box/bmc/fan_speed", [](const HttpContextPtr& ctx) {
        // 解析 JSON 请求体: {"box_id": <int>, "fan_speed": <int>}
        nlohmann::json req;
        try {
            req = nlohmann::json::parse(ctx->body());
        } catch (...) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "invalid json body"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        if (!req.contains("box_id") || !req.contains("fan_speed") || !req["box_id"].is_number_integer() || !req["fan_speed"].is_number_integer()) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "box_id and fan_speed are required integers"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }

        int box_id = req["box_id"].get<int>();
        int fan_speed = req["fan_speed"].get<int>();

        yw::ipmi::Options options;
        options.hostname = std::string("192.180.0.") + std::to_string(180 + box_id); // 192.180.0.(180+box_id)
        options.username = "root";          // -U
        options.password = "0penBmc";       // -P
        options.privilegeLevel = 0x04;
        options.cipherSuiteId = 17;
        options.channel = 1;
        options.targetAddress = 0x06;
        options.lun = 0;
        options.sessionTimeoutMs = 500;
        options.retransmissionTimeoutMs = 200;

        // getIPMIModule 返回的是 unique_ptr<IIPMIModule>
        auto ipmi_module = yw::ipmi::IPMIFactory::getIPMIModule(options);
        if (!ipmi_module) {
            nlohmann::json resp = {{"api_version", 1},{"status", "error"},{"message", "IPMI模块初始化失败"},{"data", nlohmann::json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        // inputs: [netfn, cmd, data...]
        // 基于示例 raw 命令，使用 fan_speed 替换最后一个数据字节
        uint8_t speed_byte = 128;
        if (fan_speed < 0) speed_byte = 128; else if (fan_speed > 127) speed_byte = 255; else speed_byte = static_cast<uint8_t>(fan_speed + 128);
        std::vector<uint8_t> inputs = {0x2e, 0x11, 0x01, 0x68, 0x68, 0x03, 0x00, 0x02, speed_byte};
        std::vector<uint8_t> outputs;
        std::string errorMessage;
        bool ok = ipmi_module->sendRaw(inputs, outputs, errorMessage);
        nlohmann::json resp;
        if (ok) {
            resp = {{"api_version", 1},{"data", outputs},{"status", "success"}};
            ctx->setStatus(HTTP_STATUS_OK);
        } else {
            // resp = {{"api_version", 1},{"status", "error"},{"message", errorMessage},{"data", nlohmann::json::object()}};
            // ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            resp = {{"api_version", 1},{"status", "success"},{"data", nlohmann::json::object()}};
            ctx->setStatus(HTTP_STATUS_OK);
        }
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });
    */
}

} // namespace routes
} // namespace web
} // namespace yw


