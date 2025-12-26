#include "bmc_routes.h"
// #include "ipmi/ipmi.h"  // 暂时注释掉，因为 IPMI 模块暂时不编译

#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "utils/response_builder.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;
using ResponseBuilder = yw::utils::ResponseBuilder;

namespace {
    // 辅助函数：根据 box_id 计算 box IP 地址
    std::string calculateBoxIP(int box_id) {
        return "192.168." + std::to_string(box_id * 2) + ".180";
    }

    // 辅助函数：验证 slot_id 范围
    bool validateSlotIds(const std::vector<int>& slot_ids) {
        for (int slot : slot_ids) {
            if (slot < 1 || slot > 12) {
                return false;
            }
        }
        return true;
    }

    // 辅助函数：解析并验证板卡操作请求参数
    struct BoardOperationParams {
        int box_id;
        std::vector<int> slot_id;
    };

    std::optional<BoardOperationParams> parseBoardOperationRequest(const HttpContextPtr& ctx) {
        json req;
        try {
            req = json::parse(ctx->body());
        } catch (const json::exception& e) {
            return std::nullopt;
        }

        if (!req.contains("box_id") || !req["box_id"].is_number_integer() || 
            !req.contains("slot_id") || !req["slot_id"].is_array()) {
            return std::nullopt;
        }

        int box_id = req["box_id"].get<int>();
        std::vector<int> slot_id = req["slot_id"].get<std::vector<int>>();
        
        if (!validateSlotIds(slot_id)) {
            return std::nullopt;
        }

        return BoardOperationParams{box_id, slot_id};
    }

    // 辅助函数：处理控制器操作响应
    int handleControllerResponse(const HttpContextPtr& ctx,
                                 const controller::IControllerModule::OperationResponse& response,
                                 bool acceptPartialSuccess = false) {
        bool isSuccess = (response.result == controller::IControllerModule::OperationResult::SUCCESS) ||
                        (acceptPartialSuccess && 
                         response.result == controller::IControllerModule::OperationResult::PARTIAL_SUCCESS);
        
        if (isSuccess) {
            return ResponseBuilder::sendSuccessWithReturn(ctx, json::object());
        } else {
            return ResponseBuilder::sendErrorWithReturn(ctx, response.message, HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
    }
}

void registerBMCRoutes(hv::HttpService* service,
                       bmc::IBMCModule* bmc_module,
                       controller::IControllerModule* controller_module) {
    if (!service) return;
    service->GET("/box/bmc", [bmc_module](const HttpContextPtr& ctx) {
        if (!bmc_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "bmc module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
        std::string box_id_param = ctx->param("box_id");
        if (box_id_param.empty()) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "box_id parameter is required", HTTP_STATUS_BAD_REQUEST);
        }
        std::string duration = ctx->param("duration");
        if (duration.empty()) duration = "5m";
        try {
            int box_id = std::stoi(box_id_param);
            auto info_opt = bmc_module->getBoxBMC(box_id);
            if (!info_opt.has_value()) {
                return ResponseBuilder::sendErrorWithReturn(ctx, "Box not found or no BMC data available", HTTP_STATUS_NOT_FOUND);
            }
            const auto& info = info_opt.value();
            json j;
            j["box_id"] = info.boxid;
            // 检查风扇是否存在（fanseq == 0xFF 表示风扇不存在）
            j["fan_0_speed"] = (info.fan[0].fanseq != 0xFF) ? info.fan[0].fanspeed : 0;
            j["fan_1_speed"] = (info.fan[1].fanseq != 0xFF) ? info.fan[1].fanspeed : 0;
            
            // 查询 BMC 传感器数据
            auto grouped = bmc_module->queryBMCSensor(calculateBoxIP(info.boxid), duration);
            if (grouped.empty()) {
                // 如果第一个 IP 没有数据，尝试第二个 IP
                std::string alt_ip = "192.168." + std::to_string(info.boxid * 2) + ".181";
                grouped = bmc_module->queryBMCSensor(alt_ip, duration);
            }
            j["sensor"] = grouped;
            
            return ResponseBuilder::sendSuccessWithReturn(ctx, j);
        } catch (const std::invalid_argument& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "Invalid box_id parameter: " + std::string(e.what()), HTTP_STATUS_BAD_REQUEST);
        } catch (const std::out_of_range& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "box_id out of range: " + std::string(e.what()), HTTP_STATUS_BAD_REQUEST);
        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "Internal error: " + std::string(e.what()), HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
    });

    // reset box board
    service->POST("/box/reset_board", [controller_module](const HttpContextPtr& ctx) {
        if (!controller_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "controller module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }

        auto params_opt = parseBoardOperationRequest(ctx);
        if (!params_opt.has_value()) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "invalid request: box_id and slot_id are required, slot_id must be array of integers in range 1-12", HTTP_STATUS_BAD_REQUEST);
        }

        std::string box_ip = calculateBoxIP(params_opt->box_id);
        auto response = controller_module->resetBoard(box_ip, params_opt->slot_id);
        return handleControllerResponse(ctx, response, false);
    });

    // power on box board
    service->POST("/box/poweron_board", [controller_module](const HttpContextPtr& ctx) {
        if (!controller_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "controller module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }

        auto params_opt = parseBoardOperationRequest(ctx);
        if (!params_opt.has_value()) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "invalid request: box_id and slot_id are required, slot_id must be array of integers in range 1-12", HTTP_STATUS_BAD_REQUEST);
        }

        std::string box_ip = calculateBoxIP(params_opt->box_id);
        auto response = controller_module->powerOnChassisBoards(box_ip, params_opt->slot_id);
        return handleControllerResponse(ctx, response, true);
    });

    // power off box board
    service->POST("/box/poweroff_board", [controller_module](const HttpContextPtr& ctx) {
        if (!controller_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "controller module unavailable", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }

        auto params_opt = parseBoardOperationRequest(ctx);
        if (!params_opt.has_value()) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "invalid request: box_id and slot_id are required, slot_id must be array of integers in range 1-12", HTTP_STATUS_BAD_REQUEST);
        }

        std::string box_ip = calculateBoxIP(params_opt->box_id);
        auto response = controller_module->powerOffChassisBoards(box_ip, params_opt->slot_id);
        return handleControllerResponse(ctx, response, true);
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


