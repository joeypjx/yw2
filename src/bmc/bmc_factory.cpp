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


