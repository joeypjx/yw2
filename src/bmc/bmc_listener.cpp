#include "bmc_listener.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "bmc_cache.h"

namespace yw {
namespace bmc {

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

BMCListener::~BMCListener() { stop(); }

void BMCListener::setHandler(PacketHandler handler) { handler_ = std::move(handler); }

void BMCListener::setRepository(std::unique_ptr<BMCRepository> repo) {
    repository_ = std::move(repo);
}

void BMCListener::start() {
    if (running_.exchange(true)) return;
    if (!openSocket()) {
        running_ = false;
        return;
    }
    th_ = std::thread(&BMCListener::runLoop, this);
}

void BMCListener::stop() {
    if (!running_.exchange(false)) return;
    if (th_.joinable()) th_.join();
    closeSocket();
}

bool BMCListener::openSocket() {
    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        std::perror("socket");
        return false;
    }

    int reuse = 1;
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::perror("setsockopt SO_REUSEADDR");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mcast_port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        return false;
    }

    ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(mcast_group_.c_str());
    mreq.imr_interface.s_addr = listen_ip_.empty() ? htonl(INADDR_ANY) : inet_addr(listen_ip_.c_str());
    if (setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        std::perror("setsockopt IP_ADD_MEMBERSHIP");
        return false;
    }

    return true;
}

void BMCListener::closeSocket() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

// 移除静态缓存，改为成员 unique_ptr

void BMCListener::runLoop() {
    while (running_) {
        std::uint8_t buffer[2048];
        sockaddr_in src{}; socklen_t slen = sizeof(src);
        ssize_t n = ::recvfrom(sock_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&src), &slen);
        if (n < 0) {
            if (!running_) break;
            std::perror("recvfrom");
            std::cerr << "[BMCListener] recvfrom 失败，errno=" << errno << std::endl;
            continue;
        }
        if (static_cast<size_t>(n) < sizeof(UdpInfo)) {
            // 丢弃无效包
            std::cerr << "[BMCListener] 收到包长度过小(" << n << "字节)，丢弃" << std::endl;
            continue;
        }
        const UdpInfo* pkt = reinterpret_cast<const UdpInfo*>(buffer);
        if (pkt->head != 0xA55A || pkt->tail != 0xA55A) {
            std::cerr << "[BMCListener] 包头/包尾校验失败，丢弃" << std::endl;
            continue;
        }
        if (handler_) {
            std::cerr << "[BMCListener] 调用 handler 处理 box_id=" << pkt->boxid << std::endl;
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
                std::cerr << "[BMCListener] bmc save failed: " << e.what() << std::endl;
            }
        }
    }
}

std::optional<UdpInfo> BMCListener::getBoxBMC(int box_id) const {
    if (!bmc_cache_) return std::nullopt;
    return bmc_cache_->getByBoxId(box_id);
}

std::vector<UdpInfo> BMCListener::getAllBoxBMC() const {
    if (!bmc_cache_) return {};
    return bmc_cache_->getAll();
}

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

} // namespace bmc
} // namespace yw


