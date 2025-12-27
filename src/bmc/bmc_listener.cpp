#include "bmc_listener.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <spdlog/spdlog.h>
#include "bmc_cache.h"

namespace yw {
namespace bmc {

// BMC监听器构造函数
// listen_ip: 监听IP地址（空字符串表示监听所有接口）
// mcast_group: 组播组地址
// mcast_port: 组播端口
// conninfo: 数据库连接信息（为空则不保存到数据库）
BMCListener::BMCListener(const std::string& listen_ip,
                         const std::string& mcast_group,
                         std::uint16_t mcast_port,
                         const std::string& conninfo)
    : listen_ip_(listen_ip), mcast_group_(mcast_group), mcast_port_(mcast_port) {
    if (!conninfo.empty()) {
        repository_ = std::make_unique<BMCRepository>(conninfo);
    }
    bmc_cache_ = std::make_unique<BMCCache>();
}

// 析构函数，自动停止监听器
BMCListener::~BMCListener() { stop(); }

// 设置数据包处理回调函数
// handler: 当接收到UDP数据包时调用的回调函数
void BMCListener::setHandler(PacketHandler handler) { handler_ = std::move(handler); }

// 设置BMC仓库实例（用于保存数据到数据库）
// repo: BMC仓库实例
void BMCListener::setRepository(std::unique_ptr<BMCRepository> repo) {
    repository_ = std::move(repo);
}

// 启动BMC监听器，创建UDP套接字并开始接收组播数据包
void BMCListener::start() {
    if (running_.exchange(true)) return;
    if (!openSocket()) {
        running_ = false;
        return;
    }
    th_ = std::thread(&BMCListener::runLoop, this);
}

// 停止BMC监听器，关闭套接字并等待工作线程结束
void BMCListener::stop() {
    if (!running_.exchange(false)) return;
    // 先关闭套接字，唤醒可能阻塞在 recvfrom 的线程
    closeSocket();
    if (th_.joinable()) th_.join();
}

// 打开UDP套接字并加入组播组
// 设置SO_REUSEADDR允许地址重用，设置接收超时避免阻塞
bool BMCListener::openSocket() {
    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        spdlog::error("socket() failed: {}", strerror(errno));
        return false;
    }

    int reuse = 1;
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        spdlog::warn("setsockopt(SO_REUSEADDR) failed: {}", strerror(errno));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mcast_port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        spdlog::error("bind() failed: {}", strerror(errno));
        return false;
    }

    // 设置接收超时，避免在停止时长期阻塞
    timeval tv{}; tv.tv_sec = 1; tv.tv_usec = 0;
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        spdlog::warn("setsockopt(SO_RCVTIMEO) failed: {}", strerror(errno));
    }

    ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(mcast_group_.c_str());
    mreq.imr_interface.s_addr = listen_ip_.empty() ? htonl(INADDR_ANY) : inet_addr(listen_ip_.c_str());
    if (setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        spdlog::error("setsockopt(IP_ADD_MEMBERSHIP) failed: {}", strerror(errno));
        return false;
    }

    return true;
}

// 关闭UDP套接字
void BMCListener::closeSocket() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

// 移除静态缓存，改为成员 unique_ptr

// 主循环：持续接收UDP组播数据包
// 对每个有效数据包：1)调用handler处理 2)更新缓存 3)保存到数据库
void BMCListener::runLoop() {
    while (running_) {
        std::uint8_t buffer[2048];
        sockaddr_in src{}; socklen_t slen = sizeof(src);
        ssize_t n = ::recvfrom(sock_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&src), &slen);
        if (n < 0) {
            if (!running_) break;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 超时，继续检查 running_
                continue;
            }
            if (errno == EINTR) {
                // 信号中断，重试
                continue;
            }
            // 若套接字已关闭，直接退出
            if (errno == EBADF) break;
            spdlog::error("recvfrom failed: {} (errno={})", strerror(errno), errno);
            continue;
        }
        if (static_cast<size_t>(n) < sizeof(UdpInfo)) {
            // 丢弃无效包
            spdlog::warn("[BMCListener] 收到包长度过小({}，期望{})，丢弃", n, sizeof(UdpInfo));
            continue;
        }
        const UdpInfo* pkt = reinterpret_cast<const UdpInfo*>(buffer);
        // 协议文档中：包头是0x5aa5，包尾是0x5aa5（文档第45行，可能与实际不符，通常包尾应为0xa55a）
        // 在小端模式下，按uint16_t读取：
        // - 0x5aa5 在内存中是 [0xa5, 0x5a]，按uint16_t读取是 0x5aa5
        // - 0xa55a 在内存中是 [0x5a, 0xa5]，按uint16_t读取是 0xa55a
        // 注意：文档中包尾写的是0x5aa5，但实际可能是0xa55a，这里先按文档检查，如果实际不符需要调整
        if (pkt->head != 0x5aa5 || (pkt->tail != 0x5aa5 && pkt->tail != 0xa55a)) {
            spdlog::warn("[BMCListener] 包头/包尾校验失败 (head=0x{:04x}, tail=0x{:04x})，期望 (head=0x5aa5, tail=0x5aa5或0xa55a)，丢弃", 
                         pkt->head, pkt->tail);
            continue;
        }
        if (handler_) {
            spdlog::debug("[BMCListener] 调用 handler 处理 box_id={}", pkt->boxid);
            handler_(*pkt);
        }
        // 写入缓存（按 box_id）
        if (bmc_cache_) {
            bmc_cache_->addOrUpdate(*pkt);
        }
        if (repository_) {
            try {
                repository_->save(*pkt);
            }
            catch (const std::exception& e) {
                spdlog::error("[BMCListener] bmc save failed: {}", e.what());
            }
        }
    }
}

// 获取指定机箱的BMC信息
// box_id: 机箱编号（1-9）
// 返回: BMC信息，不存在时返回std::nullopt
std::optional<UdpInfo> BMCListener::getBoxBMC(int box_id) const {
    if (!bmc_cache_) return std::nullopt;
    return bmc_cache_->getByBoxId(box_id);
}

// 获取所有机箱的BMC信息
// 返回: 所有机箱的BMC信息列表
std::vector<UdpInfo> BMCListener::getAllBoxBMC() const {
    if (!bmc_cache_) return {};
    return bmc_cache_->getAll();
}

// 查询指定节点的BMC传感器时序数据
// host_ip: 节点IP地址
// duration: 时间范围（支持简写如"1h"/"5m"/"10s"，自动转换为PostgreSQL interval格式）
// 返回: BMC传感器数据映射（传感器名称->传感器时序数据列表）
std::unordered_map<std::string, std::vector<BMCSensorRow>> BMCListener::queryBMCSensor(
    const std::string& host_ip,
    const std::string& duration) const {
    if (!repository_) return {};
    // 允许简写：1h/5m/10s（仅单一单位）。转换为 PostgreSQL interval 字符串
    auto normalizeDuration = [](std::string in) -> std::string {
        if (in.empty()) return std::string("1 minute");
        auto trim = [](std::string& s){
            size_t a = s.find_first_not_of(" \t\n\r");
            size_t b = s.find_last_not_of(" \t\n\r");
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
        };
        trim(in);
        if (in.empty()) return std::string("1 minute");
        char u = in.back();
        if (u=='s' || u=='S' || u=='m' || u=='M' || u=='h' || u=='H') {
            std::string num = in.substr(0, in.size()-1);
            trim(num);
            if (num.empty()) return std::string("1 minute");
            bool ok = true; for (char ch : num) { if (ch<'0'||ch>'9') { ok=false; break; } }
            if (!ok) return in;
            switch (u) {
                case 's': case 'S': return num + " seconds";
                case 'm': case 'M': return num + " minutes";
                case 'h': case 'H': return num + " hours";
            }
        }
        return in;
    };
    std::string interval = normalizeDuration(duration);
    return repository_->queryBMCSensor(host_ip, interval);
}

// 获取指定节点的最新BMC传感器数据（每个传感器只返回最新一条记录）
// host_ip: 节点IP地址
// 返回: BMC传感器数据映射（传感器名称->最新传感器数据）
std::unordered_map<std::string, BMCSensorRow> BMCListener::getLatestBMCSensor(
    const std::string& host_ip) const {
    if (!repository_) return {};
    return repository_->getLatestBMCSensor(host_ip);
}

// 获取指定机箱和板卡的板卡在位状态
// box_id: 机箱编号（1-9）
// board_id: 板卡编号（1-14）
// 返回: 板卡在位状态（0表示不在位，非0表示在位），不存在时返回std::nullopt
std::optional<std::uint8_t> BMCListener::getBoardPrst(int box_id, int board_id) const {
    if (!bmc_cache_) return std::nullopt;
    return bmc_cache_->getBoardPrst(box_id, board_id);
}

} // namespace bmc
} // namespace yw


