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


