#include "../../include/yw/bmc.h"
#include "bmc_listener.h"
#include <mutex>

namespace yw {
namespace bmc {

std::shared_ptr<IBMCModule> BMCFactory::getBMCModule() {
    static std::shared_ptr<IBMCModule> instance;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        // 硬编码：监听任意网卡，固定组播与端口，数据库连接可按需调整
        const std::string listen_ip = ""; // 0.0.0.0
        const std::string mcast_group = "224.100.200.15";
        const std::uint16_t mcast_port = 5715;
        const std::string conninfo = "postgres://postgres:HZ715Net@localhost:5432/yw";
        auto listener = std::make_shared<BMCListener>(listen_ip, mcast_group, mcast_port, conninfo);
        listener->start();
        instance = listener;
    }
    return instance;
}

} // namespace bmc
} // namespace yw


