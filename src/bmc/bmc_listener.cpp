#include "bmc_listener.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

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

void BMCListener::runLoop() {
    while (running_) {
        std::uint8_t buffer[2048];
        sockaddr_in src{}; socklen_t slen = sizeof(src);
        ssize_t n = ::recvfrom(sock_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&src), &slen);
        if (n < 0) {
            if (!running_) break;
            std::perror("recvfrom");
            continue;
        }
        if (static_cast<size_t>(n) < sizeof(UdpInfo)) {
            // 丢弃无效包
            continue;
        }
        const UdpInfo* pkt = reinterpret_cast<const UdpInfo*>(buffer);
        if (pkt->head != 0xA55A || pkt->tail != 0xA55A) {
            continue;
        }
        if (handler_) handler_(*pkt);
        if (repository_) {
            try { repository_->save(*pkt); }
            catch (const std::exception& e) { std::cerr << "bmc save failed: " << e.what() << std::endl; }
        }
    }
}

std::unordered_map<std::string, std::vector<BMCSensorRow>> BMCListener::queryBMCSensor(
    const std::string& host_ip,
    const std::string& duration) const {
    if (!repository_) return {};
    return repository_->queryBMCSensor(host_ip, duration);
}

} // namespace bmc
} // namespace yw


