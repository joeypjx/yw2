#include "bmc_cache.h"
#include <chrono>

namespace yw {
namespace bmc {

// 添加或更新BMC缓存数据
// info: UDP组播接收到的BMC信息
// 返回: 成功返回true，失败返回false
bool BMCCache::addOrUpdate(const UdpInfo& info) {
    // UdpInfo.boxid 是 std::uint8_t，根据需求按 box_id 作为 key
    int key = static_cast<int>(info.boxid);
    if (key < 0) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();
    Record& rec = cache_[key];
    rec.info = info;
    rec.last_update_ms = static_cast<std::int64_t>(now_ms);
    return true;
}

// 根据机箱号获取BMC信息
// box_id: 机箱编号（1-9）
// 返回: BMC信息对象，不存在时返回std::nullopt
std::optional<UdpInfo> BMCCache::getByBoxId(int box_id) const {
    if (box_id < 0) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(box_id);
    if (it == cache_.end()) return std::nullopt;
    return it->second.info;
}

// 获取所有缓存的BMC信息
// 返回: 所有BMC信息的向量
std::vector<UdpInfo> BMCCache::getAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<UdpInfo> out;
    out.reserve(cache_.size());
    for (const auto& kv : cache_) {
        out.push_back(kv.second.info);
    }
    return out;
}

// 获取板卡在位状态
// box_id: 机箱编号
// board_id: 板卡IPMB地址
// 返回: 板卡在位状态（0=不在位，1=在位），数据不存在或超时返回0
std::optional<std::uint8_t> BMCCache::getBoardPrst(int box_id, int board_id) const {
    if (box_id < 0 || board_id < 0) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(box_id);
    // TODO:机箱数据不存在，则认为所有板卡不在位
    if (it == cache_.end()) return std::nullopt;

    // 机箱数据超过20秒未更新，则认为所有板卡不在位
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();
    if (it->second.last_update_ms < static_cast<std::int64_t>(now_ms) - 20000) {
        return 0;
    } else {
        for (int i = 0; i < 10; i++) {
            if (it->second.info.board[i].ipmbaddr == board_id) {
                return it->second.info.board[i].prst;
            }
        }
    }

    return 0;
}

} // namespace bmc
} // namespace yw

