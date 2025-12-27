#include "bmc_routes.h"
#include "ipmi/ipmi.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "utils/response_builder.h"
#include "utils/ip_address_utils.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;
using ResponseBuilder = yw::utils::ResponseBuilder;

namespace {
    // 辅助函数：根据 box_id 计算 box IP 地址（使用槽位7的IP）
    // box_id: 机箱编号（1-9）
    // 返回: 机箱IP地址（槽位7的IP）
    std::string calculateBoxIP(int box_id) {
        return yw::utils::IPAddressUtils::calculateHostIP(box_id, 7);
    }

    // 辅助函数：验证 slot_id 范围
    // slot_ids: 槽位ID列表
    // 返回: 所有槽位ID都在有效范围（1-12）内返回true，否则返回false
    bool validateSlotIds(const std::vector<int>& slot_ids) {
        for (int slot : slot_ids) {
            if (slot < 1 || slot > 12) {
                return false;
            }
        }
        return true;
    }

    // 辅助函数：解析并验证板卡操作请求参数
    // ctx: HTTP上下文
    // 返回: 解析成功返回参数对象，失败返回std::nullopt
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
    // ctx: HTTP上下文
    // response: 控制器操作响应
    // acceptPartialSuccess: 是否接受部分成功（默认false）
    // 返回: HTTP状态码
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

// 注册BMC相关的HTTP路由
// service: HTTP服务实例
// bmc_module: BMC模块实例
// controller_module: 控制器模块实例
void registerBMCRoutes(hv::HttpService* service,
                       bmc::IBMCModule* bmc_module,
                       controller::IControllerModule* controller_module) {
    if (!service) return;
    
    // GET /box/bmc - 获取指定机箱的BMC数据
    // 查询参数：box_id（必需）- 机箱编号
    // 查询参数：duration（可选，默认"5m"）- 时间范围
    // 返回：机箱BMC数据JSON对象（包含风扇速度、传感器数据等）
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

    // POST /box/reset_board - 重置机箱板卡
    // 请求体：{"box_id": 1, "slot_id": [1, 2, 3]}
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

    // POST /box/poweron_board - 开启机箱板卡电源
    // 请求体：{"box_id": 1, "slot_id": [1, 2, 3]}
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

    // POST /box/poweroff_board - 关闭机箱板卡电源
    // 请求体：{"box_id": 1, "slot_id": [1, 2, 3]}
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

    // POST /box/bmc/fan_speed - 设置机箱风扇速度
    // 请求体: {"box_id": <int>, "fan_speed": <int>}
    // fan_speed 范围: 0-127，对应实际速度值 128-255
    service->POST("/box/bmc/fan_speed", [](const HttpContextPtr& ctx) {
        // 1. 解析 JSON 请求体
        nlohmann::json req;
        try {
            req = nlohmann::json::parse(ctx->body());
        } catch (...) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "invalid json body", HTTP_STATUS_BAD_REQUEST);
        }
        
        // 2. 验证请求参数
        if (!req.contains("box_id") || !req.contains("fan_speed") || 
            !req["box_id"].is_number_integer() || !req["fan_speed"].is_number_integer()) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "box_id and fan_speed are required integers", HTTP_STATUS_BAD_REQUEST);
        }

        int box_id = req["box_id"].get<int>();
        int fan_speed = req["fan_speed"].get<int>();

        // 3. 配置 IPMI 连接选项
        yw::ipmi::Options options;
        // BMC 主机地址：192.180.0.(180+box_id)，例如 box_id=1 对应 192.180.0.181
        options.hostname = std::string("192.180.0.") + std::to_string(180 + box_id);
        options.username = "root";          // IPMI 用户名
        options.password = "0penBmc";         // IPMI 密码
        options.privilegeLevel = 0x04;        // 管理员权限级别
        options.cipherSuiteId = 17;          // 加密套件 ID
        options.channel = 1;                 // IPMI 通道号
        options.targetAddress = 0x06;        // 目标地址（BMC 地址）
        options.lun = 0;                     // 逻辑单元号
        options.sessionTimeoutMs = 500;      // 会话超时时间（毫秒）
        options.retransmissionTimeoutMs = 200; // 重传超时时间（毫秒）

        // 4. 创建 IPMI 模块实例
        auto ipmi_module = yw::ipmi::IPMIFactory::getIPMIModule(options);
        if (!ipmi_module) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "IPMI模块初始化失败", HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
        
        // 5. 计算风扇速度字节值
        // IPMI 协议中，风扇速度值范围是 128-255
        // 用户输入范围是 0-127，需要加上 128 进行转换
        // 小于 0 的值映射到 128（最低速度），大于 127 的值映射到 255（最高速度）
        uint8_t speed_byte = 128;
        if (fan_speed < 0) {
            speed_byte = 128;  // 最低速度
        } else if (fan_speed > 127) {
            speed_byte = 255;  // 最高速度
        } else {
            speed_byte = static_cast<uint8_t>(fan_speed + 128);  // 正常范围：128-255
        }
        
        // 6. 构造 IPMI 原始命令
        // 命令格式: [netfn, cmd, data...]
        // 0x2e: Network Function (OEM/Group Extension)
        // 0x11: Command (设置风扇速度)
        // 后续字节: 命令参数，最后一个字节是速度值
        std::vector<uint8_t> inputs = {0x2e, 0x11, 0x01, 0x68, 0x68, 0x03, 0x00, 0x02, speed_byte};
        
        // 7. 发送 IPMI 命令并获取响应
        std::vector<uint8_t> outputs;
        std::string errorMessage;
        bool ok = ipmi_module->sendRaw(inputs, outputs, errorMessage);
        
        // 8. 处理响应结果
        if (ok) {
            // 成功：将响应字节数组转换为 JSON 数组返回
            nlohmann::json outputs_json = nlohmann::json::array();
            for (uint8_t byte : outputs) {
                outputs_json.push_back(byte);
            }
            return ResponseBuilder::sendSuccessWithReturn(ctx, outputs_json);
        } else {
            // 失败：返回空对象（不暴露内部错误信息，避免安全风险）
            return ResponseBuilder::sendSuccessWithReturn(ctx, nlohmann::json::object());
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw


