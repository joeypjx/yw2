#ifndef RESOURCE_CONTROLLER_H
#define RESOURCE_CONTROLLER_H

#include "controller/controller.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <cstdio>

namespace yw {
namespace controller {

// 资源控制器，负责板卡的重启、上下电等操作
class ResourceController : public IControllerModule {
public:
    using BinaryData = std::vector<uint8_t>;

    // 构造函数
    ResourceController();
    // 析构函数
    ~ResourceController() override = default;

    // 实现 IControllerModule 接口
    // 重启指定槽位的板卡
    // target_ip: 目标机箱IP地址
    // slot_numbers: 要重启的槽位号列表
    // req_id: 请求ID（可选）
    // 返回: 操作响应结果
    OperationResponse resetBoard(
        const std::string& target_ip,
        const std::vector<int>& slot_numbers,
        uint32_t req_id = 0) override;

    // 关闭指定槽位的板卡电源
    // target_ip: 目标机箱IP地址
    // slot_numbers: 要关闭的槽位号列表
    // req_id: 请求ID（可选）
    // 返回: 操作响应结果
    OperationResponse powerOffChassisBoards(
        const std::string& target_ip,
        const std::vector<int>& slot_numbers,
        uint32_t req_id = 0) override;
    
    // 开启指定槽位的板卡电源
    // target_ip: 目标机箱IP地址
    // slot_numbers: 要开启的槽位号列表
    // req_id: 请求ID（可选）
    // 返回: 操作响应结果
    OperationResponse powerOnChassisBoards(
        const std::string& target_ip,
        const std::vector<int>& slot_numbers,
        uint32_t req_id = 0) override;

    // 工具方法
    // 将二进制数据转换为十六进制字符串
    static std::string binaryToHex(const BinaryData& data) {
        std::string hex_string;
        hex_string.reserve(data.size() * 2);
        for (uint8_t byte : data) {
            char hex[3];
            std::snprintf(hex, sizeof(hex), "%02x", byte);
            hex_string += hex;
        }
        return hex_string;
    }

private:
    // 内部协议结构，仅类内使用
    struct OperationModel {
        char m_strFlag[8];
        char m_strIp[16];
        char m_cmd[8];
        char m_slot[16];
        uint32_t m_reqId;
    };
    // 固定参数：发送到 33000，本地监听 33001，超时默认 10 秒，标识符 "ETHSWB"
    int server_port_ = 33000;
    int receive_port_ = 33001;
    int timeout_seconds_ = 10;
    std::string operation_flag_ = "ETHSWB";

    // 内部方法：执行操作
    OperationResponse executeOperation(const std::string& cmd,
                                     const std::string& target_ip,
                                     const std::vector<int>& slot_numbers,
                                     uint32_t req_id);

    // 内部方法：构建操作模型
    OperationModel buildOperationModel(const std::string& cmd,
                                      const std::string& target_ip,
                                      const std::vector<int>& slot_numbers,
                                      uint32_t req_id) const;

    // 内部方法：解析响应
    OperationResult parseResponse(const BinaryData& response, 
                                  const std::vector<int>& slot_numbers,
                                 std::vector<SlotResult>& slot_results,
                                 std::string& message) const;
};

} // namespace controller
} // namespace yw

#endif // RESOURCE_CONTROLLER_H

