// ============================================================================
// 文件功能描述：
// IPMI模块（IIPMIModule）的头文件，定义IPMI通信模块的公共接口。
// 主要功能包括：
// 1. IPMI连接选项：定义IPMI连接所需的配置选项（主机名、用户名、密码、权限级别等）
// 2. 原始命令接口：提供sendRaw方法，支持发送任意IPMI原始命令
// 3. 工厂模式：提供IPMIFactory工厂类，用于创建IPMI模块实例
// 4. 接口抽象：定义IIPMIModule接口，隐藏具体实现细节，实现模块间的松耦合
// ============================================================================

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace yw {
namespace ipmi {

struct Options {
    std::string hostname;
    std::string username;
    std::string password;
    uint8_t privilegeLevel = 0x04; // ADMIN
    uint8_t cipherSuiteId = 17;    // ipmitool -C17 默认映射
    uint8_t channel = 1;           // -b 1
    uint8_t targetAddress = 0x06;  // -t 0x06
    uint8_t lun = 0;               // 默认 0
    unsigned int sessionTimeoutMs = 500;
    unsigned int retransmissionTimeoutMs = 200;
};

class IIPMIModule {
public:
    virtual ~IIPMIModule() = default;

    // inputs: [netfn, cmd, data...]
    // outputs: 原始响应字节数组
    virtual bool sendRaw(const std::vector<uint8_t>& inputs,
                         std::vector<uint8_t>& outputs,
                         std::string& errorMessage) = 0;
};

class IPMIFactory {
public:
    static std::unique_ptr<IIPMIModule> getIPMIModule(const Options& options);
};

} // namespace ipmi
} // namespace yw


