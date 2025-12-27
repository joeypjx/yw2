// ============================================================================
// 文件功能描述：
// IPMI客户端（IpmiClient）的头文件，定义IPMI通信客户端的接口。
// 主要功能包括：
// 1. IPMI连接管理：封装FreeIPMI库，提供IPMI连接的创建和销毁
// 2. 原始命令发送：提供sendRaw方法，支持发送任意IPMI原始命令
// 3. 错误处理：提供错误信息输出参数，便于调试和错误处理
// 4. PIMPL模式：使用Impl结构体隐藏实现细节，减少头文件依赖
// ============================================================================

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


