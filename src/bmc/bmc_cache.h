#pragma once

#include <unordered_map>
#include <optional>
#include <mutex>
#include <cstdint>
#include <vector>

#include "bmc/bmc_model.h"

namespace yw {
namespace bmc {

/**
 * @brief BMC 数据缓存，按 box_id 缓存最近一次 UdpInfo
 */
class BMCCache {
public:
    BMCCache() = default;
    ~BMCCache() = default;

    BMCCache(const BMCCache&) = delete;
    BMCCache& operator=(const BMCCache&) = delete;

    /**
     * @brief 新增或更新指定 box_id 的 UdpInfo
     */
    bool addOrUpdate(const UdpInfo& info);

    /**
     * @brief 根据 box_id 获取最近一次的 UdpInfo
     */
    std::optional<UdpInfo> getByBoxId(int box_id) const;

    /**
     * @brief 获取全部的 UdpInfo（拷贝）
     */
    std::vector<UdpInfo> getAll() const;

    /**
     * @brief 根据 box_id 和 board_id 获取板卡在位信息
     */
    std::optional<std::uint8_t> getBoardPrst(int box_id, int board_id) const;

private:
    struct Record {
        UdpInfo info{};
        std::int64_t last_update_ms = 0;
    };

    mutable std::mutex mutex_;
    std::unordered_map<int, Record> cache_;
};

} // namespace bmc
} // namespace yw

