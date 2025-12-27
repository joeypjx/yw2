#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ipmi/ipmi.h"

namespace yw {
namespace ipmi {

// IPMI客户端，基于FreeIPMI库实现IPMI通信
class IpmiClient : public IIPMIModule {
public:
    // 构造函数，初始化IPMI连接
    // options: IPMI连接选项
    explicit IpmiClient(const Options &options);
    // 析构函数，关闭IPMI连接
    ~IpmiClient() override;

    // 发送原始IPMI命令
    // inputs: 输入字节数组
    // outputs: 输出参数，接收响应数据
    // errorMessage: 输出参数，错误信息
    // 返回: 成功返回true，失败返回false
    bool sendRaw(const std::vector<uint8_t> &inputs, std::vector<uint8_t> &outputs, std::string &errorMessage) override;

private:
    struct Impl;
    Impl *impl_;
};

} // namespace ipmi
} // namespace yw


