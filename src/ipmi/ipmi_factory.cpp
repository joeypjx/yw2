// ============================================================================
// 文件功能描述：
// IPMI工厂（IPMIFactory）的实现文件，负责创建和初始化IPMI模块实例。
// 主要功能包括：
// 1. 模块创建：根据IPMI连接选项创建IpmiClient实例，封装FreeIPMI库功能
// 2. 工厂模式：提供统一的模块创建接口，隐藏具体的实现细节
// 3. 配置注入：接收IPMI连接选项（主机名、用户名、密码等），实现灵活的配置
// ============================================================================

#include "ipmi/ipmi.h"
#include "ipmi_client.h"
#include <memory>

namespace yw { namespace ipmi {

// 创建IPMI模块实例
// options: IPMI连接选项（主机名、用户名、密码等）
// 返回: IPMI模块唯一指针
std::unique_ptr<IIPMIModule> IPMIFactory::getIPMIModule(const Options& options)
{
    return std::make_unique<IpmiClient>(options);
}

}} // namespace yw::ipmi


