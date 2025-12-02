#include "bmc_cache.h"
#include <chrono>

namespace yw {
namespace bmc {

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

std::optional<UdpInfo> BMCCache::getByBoxId(int box_id) const {
    if (box_id < 0) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(box_id);
    if (it == cache_.end()) return std::nullopt;
    return it->second.info;
}

std::vector<UdpInfo> BMCCache::getAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<UdpInfo> out;
    out.reserve(cache_.size());
    for (const auto& kv : cache_) {
        out.push_back(kv.second.info);
    }
    return out;
}

std::optional<std::uint8_t> BMCCache::getBoardPrst(int box_id, int board_id) const {
    if (box_id < 0 || board_id < 0) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(box_id);
    if (it == cache_.end()) return std::nullopt;

    for (int i = 0; i < 10; i++) {
        if (it->second.info.board[i].ipmbaddr == board_id) {
            return it->second.info.board[i].prst;
        }
    }
    return 0;
}

} // namespace bmc
} // namespace yw

