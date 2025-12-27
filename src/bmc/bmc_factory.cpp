// ============================================================================
// 文件功能描述：
// BMC工厂（BMCFactory）的实现文件，负责创建和初始化BMC模块实例。
// 主要功能包括：
// 1. 模块创建：从配置文件读取BMC监听参数，创建BMCListener实例
// 2. 配置加载：读取组播IP、端口、监听IP和数据库连接信息等配置项
// 3. 自动启动：创建BMC监听器后自动启动，开始接收UDP组播数据包
// 4. 工厂模式：提供统一的模块创建接口，隐藏具体的实现细节
// ============================================================================

#include "bmc/bmc.h"
#include "bmc_listener.h"
#include "utils/json_config.h"
#include <mutex>

namespace yw {
namespace bmc {

// 创建BMC模块实例
// 从配置文件加载BMC监听参数和数据库连接信息，创建并启动BMC监听器
// 返回: BMC模块共享指针
std::shared_ptr<IBMCModule> BMCFactory::getBMCModule() {
    // 从配置加载 BMC 与数据库参数
    const std::string listen_ip   = yw::utils::JsonConfig::Get<std::string>("bmc.listen_ip", "");
    const std::string mcast_group = yw::utils::JsonConfig::Get<std::string>("bmc.multicast_group", "224.100.200.15");
    const std::uint16_t mcast_port = static_cast<std::uint16_t>(yw::utils::JsonConfig::Get<int>("bmc.multicast_port", 5715));
    const std::string conninfo    = yw::utils::JsonConfig::Get<std::string>("db.conninfo", "postgres://postgres:HZ715Net@localhost:5432/yw");
    auto listener = std::make_shared<BMCListener>(listen_ip, mcast_group, mcast_port, conninfo);
    listener->start();
    return listener;
}

} // namespace bmc
} // namespace yw


