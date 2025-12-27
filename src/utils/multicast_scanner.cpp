#include "utils/multicast_scanner.h"
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

// 组播扫描器构造函数
// manager_ip: 管理节点IP地址
// manager_port: 管理节点端口
// url: 服务URL路径
// multicast_ip: 组播IP地址
// multicast_port: 组播端口
// interval_ms: 发送间隔（毫秒）
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

// 析构函数，自动停止扫描器
MulticastScanner::~MulticastScanner() {
    stop();
}

// 启动组播扫描器，创建UDP套接字并开始周期性发送组播消息
void MulticastScanner::start() {
    if (running_.exchange(true)) return;
    if (!openSocket()) {
        running_ = false;
        return;
    }
    worker_ = std::thread(&MulticastScanner::runLoop, this);
}

// 停止组播扫描器，关闭套接字并等待工作线程结束
void MulticastScanner::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
    closeSocket();
}

// 设置发送间隔时间
// interval_ms: 新的间隔时间（毫秒）
void MulticastScanner::setIntervalMs(int interval_ms) {
    interval_ms_ = interval_ms;
}

// 主循环：周期性发送组播消息
// 每次发送后等待指定间隔时间，直到stop()被调用
void MulticastScanner::runLoop() {
    while (running_) {
        (void)sendOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

// 发送一次组播消息
// 构造包含管理节点信息的JSON消息，通过UDP组播发送
// 返回: 发送成功返回true，失败返回false（网络错误时会关闭套接字）
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

    return true;
}

// 打开UDP套接字并配置组播选项
// 设置组播接口（如果指定了manager_ip）和TTL值
// 返回: 成功返回true，失败返回false
bool MulticastScanner::openSocket() {
    closeSocket();
    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        spdlog::error("MulticastScanner socket() failed: {}", strerror(errno));
        return false;
    }

    if (!manager_ip_.empty()) {
        in_addr ifaddr{};
        ifaddr.s_addr = inet_addr(manager_ip_.c_str());
        setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr));
    }

    int ttl = 1;
    if (setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        spdlog::warn("MulticastScanner setsockopt TTL failed: {}", strerror(errno));
    }

    // 目的地址在发送时现构造（避免在头文件暴露平台相关类型）
    return true;
}

// 关闭UDP套接字
void MulticastScanner::closeSocket() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

} // namespace utils
} // namespace yw


