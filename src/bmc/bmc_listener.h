// ============================================================================
// 文件功能描述：
// BMC监听器（BMCListener）的头文件，定义BMC UDP组播监听服务的接口和实现。
// 主要功能包括：
// 1. UDP组播监听：实现IBMCModule接口，接收BMC设备通过UDP组播发送的传感器数据
// 2. 数据包解析：解析UDP数据包，验证包头包尾，提取BMC传感器信息
// 3. 数据缓存：将接收到的BMC数据更新到内存缓存（BMCCache）
// 4. 数据持久化：将BMC数据保存到PostgreSQL数据库（TimescaleDB）
// 5. 查询接口：提供查询BMC传感器时序数据和最新数据的功能
// 6. 板卡状态查询：提供查询板卡在位状态（PRST）的功能
// 7. 线程管理：使用独立线程运行接收循环，支持优雅启动和停止
// ============================================================================

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

// BMC监听器，监听UDP组播消息并处理BMC数据
class BMCListener : public IBMCModule {
public:
    using PacketHandler = std::function<void(const UdpInfo&)>;

    // 构造函数，初始化BMC监听器
    // listen_ip: 监听IP地址（空字符串表示监听所有接口）
    // mcast_group: 组播组地址
    // mcast_port: 组播端口
    // conninfo: 数据库连接信息
    BMCListener(const std::string& listen_ip,
                const std::string& mcast_group = "224.100.200.15",
                std::uint16_t mcast_port = 5715,
                const std::string& conninfo = "");
    // 析构函数，停止监听并清理资源
    ~BMCListener();

    // 启动BMC监听器，开始接收UDP组播消息
    void start();
    // 停止BMC监听器
    void stop();

    // 设置数据包处理回调函数
    void setHandler(PacketHandler handler);
    // 设置BMC仓库实例
    void setRepository(std::unique_ptr<BMCRepository> repo);

    // 获取板卡在位状态
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


