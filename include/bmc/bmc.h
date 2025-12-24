#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <optional>

#include "bmc/bmc_model.h"

namespace yw {
namespace bmc {

/**
 * @brief BMC 模块接口
 * 
 * 提供 BMC (Baseboard Management Controller) 相关的数据查询功能，
 * 包括板卡状态、传感器数据、BMC 信息等。
 */
class IBMCModule {
public:
    virtual ~IBMCModule() = default;

    // ========== BMC 信息查询 ==========

    /**
     * @brief 获取所有机箱的最新 BMC 信息
     * 
     * @return 所有机箱的 BMC UdpInfo 列表
     */
    virtual std::vector<UdpInfo> getAllBoxBMC() const = 0;

    /**
     * @brief 获取指定机箱的 BMC 信息
     * 
     * @param box_id 机箱号（通常范围 1-9）
     * @return 机箱的 BMC UdpInfo 信息，如果不存在返回 std::nullopt
     */
    virtual std::optional<UdpInfo> getBoxBMC(int box_id) const = 0;

    // ========== 板卡状态查询 ==========

    /**
     * @brief 获取指定板卡的在位状态
     * 
     * @param box_id 机箱号（通常范围 1-9）
     * @param board_id 板卡槽位号（通常范围 1-12，排除 5 和 8）
     * @return 板卡在位状态：0 表示不在位，1 表示在位；如果查询失败返回 std::nullopt
     */
    virtual std::optional<std::uint8_t> getBoardPrst(int box_id, int board_id) const = 0;

    // ========== BMC 传感器数据查询 ==========

    /**
     * @brief 获取指定主机的最新 BMC 传感器数据
     * 
     * 每个传感器只返回最新的一条记录，用于快速获取当前状态。
     * 
     * @param host_ip 主机 IP 地址
     * @return 按传感器名称索引的最新传感器数据，key 为传感器名称，value 为该传感器的最新数据
     */
    virtual std::unordered_map<std::string, BMCSensorRow> getLatestBMCSensor(
        const std::string& host_ip) const = 0;

    /**
     * @brief 查询指定主机在指定时间范围内的 BMC 传感器数据
     * 
     * @param host_ip 主机 IP 地址
     * @param duration 时间范围，支持格式：数字+单位（如 "5m", "1h", "30s"）或 PostgreSQL interval 格式
     * @return 按传感器名称分组的传感器数据列表，key 为传感器名称，value 为该传感器的历史数据
     */
    virtual std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration) const = 0;
};

/**
 * @brief BMC 模块工厂类
 * 
 * 用于创建和获取 BMC 模块实例。
 */
class BMCFactory {
public:
    /**
     * @brief 获取 BMC 模块实例
     * 
     * @return BMC 模块的共享指针，如果创建失败可能返回 nullptr
     */
    static std::shared_ptr<IBMCModule> getBMCModule();
};

} // namespace bmc
} // namespace yw