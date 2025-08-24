#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>

#include "yw/bmc_model.h"
#include "bmc_repository.h"
#include "yw/bmc.h"

namespace yw {
namespace bmc {

class BMCListener : public IBMCModule {
public:
    using PacketHandler = std::function<void(const UdpInfo&)>;

    BMCListener(const std::string& listen_ip,
                const std::string& mcast_group = "224.100.200.15",
                std::uint16_t mcast_port = 5715,
                const std::string& conninfo = "");
    ~BMCListener();

    void start();
    void stop();

    void setHandler(PacketHandler handler);
    void setRepository(std::unique_ptr<BMCRepository> repo);

    // IBMCModule 接口实现
    std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration) const override;

private:
    void runLoop();
    bool openSocket();
    void closeSocket();

private:
    std::string listen_ip_;
    std::string mcast_group_;
    std::uint16_t mcast_port_;

    int sock_ = -1;
    std::thread th_;
    std::atomic<bool> running_{false};
    PacketHandler handler_;
    std::unique_ptr<BMCRepository> repository_;
};

} // namespace bmc
} // namespace yw


