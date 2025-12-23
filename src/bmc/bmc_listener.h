#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include <memory>
#include <optional>

#include "bmc/bmc_model.h"
#include "bmc_repository.h"
#include "bmc/bmc.h"
#include "bmc_cache.h"

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

    std::optional<std::uint8_t> getBoardPrst(int box_id, int board_id) const override;

    // IBMCModule 接口实现
    std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration) const override;

    std::unordered_map<std::string, BMCSensorRow> getLatestBMCSensor(
        const std::string& host_ip) const override;

    // BMCCache 访问接口（查询指定 box_id 的 UdpInfo）
    std::optional<UdpInfo> getBoxBMC(int box_id) const override;

    // 获取所有 box 的最新 BMC UdpInfo
    std::vector<UdpInfo> getAllBoxBMC() const override;

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

    // 缓存（按 box_id）
    std::unique_ptr<BMCCache> bmc_cache_;
};

} // namespace bmc
} // namespace yw


