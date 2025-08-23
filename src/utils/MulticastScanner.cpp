#include "yw/MulticastScanner.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace yw {
namespace utils {

using json = nlohmann::json;

MulticastScanner::MulticastScanner(const std::string& manager_ip,
                                   int manager_port,
                                   std::string url,
                                   std::string multicast_ip,
                                   int multicast_port,
                                   int interval_ms)
    : manager_ip_(manager_ip),
      manager_port_(manager_port),
      url_(std::move(url)),
      multicast_ip_(std::move(multicast_ip)),
      multicast_port_(multicast_port),
      interval_ms_(interval_ms) {}

MulticastScanner::~MulticastScanner() {
    stop();
}

void MulticastScanner::start() {
    if (running_.exchange(true)) return;
    if (!openSocket()) {
        running_ = false;
        return;
    }
    worker_ = std::thread(&MulticastScanner::runLoop, this);
}

void MulticastScanner::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
    closeSocket();
}

void MulticastScanner::setIntervalMs(int interval_ms) {
    interval_ms_ = interval_ms;
}

void MulticastScanner::runLoop() {
    while (running_) {
        (void)sendOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

bool MulticastScanner::sendOnce() {
    if (sock_ < 0) {
        if (!openSocket()) return false;
    }

    json j;
    j["api_version"] = 1;
    j["data"] = {
        {"manager_ip", manager_ip_},
        {"manager_port", manager_port_},
        {"url", url_}
    };

    const std::string payload = j.dump();
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(static_cast<uint16_t>(multicast_port_));
    dest.sin_addr.s_addr = inet_addr(multicast_ip_.c_str());
    ssize_t n = sendto(sock_, payload.data(), payload.size(), 0,
                       reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    if (n < 0) {
        spdlog::warn("MulticastScanner sendto failed: {}", strerror(errno));
        if (errno == ENETDOWN || errno == EADDRNOTAVAIL || errno == ENETUNREACH) {
            closeSocket();
        }
        return false;
    }

    spdlog::info("MulticastScanner multicast sent: {} bytes to {}:{}", n, multicast_ip_, multicast_port_);
    return true;
}

bool MulticastScanner::openSocket() {
    closeSocket();
    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        spdlog::error("MulticastScanner socket() failed: {}", strerror(errno));
        return false;
    }

    if (!manager_ip_.empty()) {
        sockaddr_in src{};
        src.sin_family = AF_INET;
        src.sin_port = htons(0);
        src.sin_addr.s_addr = inet_addr(manager_ip_.c_str());
        if (bind(sock_, reinterpret_cast<sockaddr*>(&src), sizeof(src)) < 0) {
            spdlog::warn("MulticastScanner bind({}) failed: {}", manager_ip_, strerror(errno));
        }
    }

    int ttl = 1;
    if (setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        spdlog::warn("MulticastScanner setsockopt TTL failed: {}", strerror(errno));
    }

    // 目的地址在发送时现构造（避免在头文件暴露平台相关类型）
    return true;
}

void MulticastScanner::closeSocket() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

} // namespace utils
} // namespace yw


