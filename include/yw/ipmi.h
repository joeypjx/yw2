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


