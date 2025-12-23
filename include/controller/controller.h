#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace yw {
namespace controller {

    /**
     * @brief 资源控制器模块接口
     * 
     * 提供机箱板卡控制操作的公共接口
     */
    class IControllerModule {
    public:
        virtual ~IControllerModule() = default;

        /**
         * @brief 操作结果枚举
         */
        enum class OperationResult {
            SUCCESS,
            PARTIAL_SUCCESS,
            NETWORK_ERROR,
            TIMEOUT_ERROR,
            INVALID_RESPONSE,
            UNKNOWN_ERROR
        };

        /**
         * @brief 槽位状态枚举
         */
        enum class SlotStatus {
            NO_OPERATION_OR_SUCCESS = 0,    // 不操作或操作成功
            REQUEST_OPERATION_OR_FAILED = 1 // 请求操作或操作失败
        };

        /**
         * @brief 槽位操作结果
         */
        struct SlotResult {
            int slot_number;     // 槽位号 (1-12)
            SlotStatus status;   // 槽位状态
        };

        /**
         * @brief 操作响应
         */
        struct OperationResponse {
            OperationResult result;
            std::string message;
            std::vector<SlotResult> slot_results; // 各槽位的操作结果
        };

        /**
         * @brief 复位板卡
         * @param target_ip 目标IP地址
         * @param slot_numbers 槽位号列表（1-12）
         * @param req_id 请求ID（可选，默认为0）
         * @return 操作响应
         */
        virtual OperationResponse resetBoard(
            const std::string& target_ip,
            const std::vector<int>& slot_numbers,
            uint32_t req_id = 0) = 0;

        /**
         * @brief 板卡下电
         * @param target_ip 目标IP地址
         * @param slot_numbers 槽位号列表（1-12）
         * @param req_id 请求ID（可选，默认为0）
         * @return 操作响应
         */
        virtual OperationResponse powerOffChassisBoards(
            const std::string& target_ip,
            const std::vector<int>& slot_numbers,
            uint32_t req_id = 0) = 0;

        /**
         * @brief 板卡上电
         * @param target_ip 目标IP地址
         * @param slot_numbers 槽位号列表（1-12）
         * @param req_id 请求ID（可选，默认为0）
         * @return 操作响应
         */
        virtual OperationResponse powerOnChassisBoards(
            const std::string& target_ip,
            const std::vector<int>& slot_numbers,
            uint32_t req_id = 0) = 0;

        /**
         * @brief 自检板卡IP地址检查连通性
         * @param ipAddress 板卡IP地址
         * @return true表示ping通，false表示ping不通
         */
        virtual bool selfcheckBoard(const std::string& ipAddress) = 0;
    };

    /**
     * @brief 资源控制器模块工厂
     * 
     * 负责创建资源控制器模块实例
     */
    class ControllerFactory {
    public:
        /**
         * @brief 创建资源控制器模块
         * @return 资源控制器模块智能指针
         */
        static std::shared_ptr<IControllerModule> getControllerModule();
    };

} // namespace controller
} // namespace yw


