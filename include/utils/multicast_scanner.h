// ============================================================================
// 文件功能描述：
// 组播扫描器（MulticastScanner）的头文件，定义通过UDP组播主动发现节点的接口。
// 主要功能包括：
// 1. 组播消息发送：提供周期性通过UDP组播发送管理节点信息的功能
// 2. 节点发现：网络中的节点接收到组播消息后，主动向管理节点注册或上报数据
// 3. 生命周期管理：提供start和stop方法，管理扫描器的启动和停止
// 4. 间隔配置：提供setIntervalMs方法，配置发送间隔时间
// 5. 线程安全：使用独立线程运行发送循环，支持优雅启动和停止
// ============================================================================

#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <cstdint>

namespace yw {
namespace utils {

class MulticastScanner {
public:
    MulticastScanner(const std::string& manager_ip,
                     int manager_port,
                     std::string url = "/heartbeat",
                     std::string multicast_ip = "239.192.168.80",
                     int multicast_port = 3980,
                     int interval_ms = 3000);
    ~MulticastScanner();

    void start();
    void stop();
    void setIntervalMs(int interval_ms);

private:
    void runLoop();
    bool sendOnce();
    bool openSocket();
    void closeSocket();

private:
    std::string manager_ip_;
    int         manager_port_ = 0;
    std::string url_;
    std::string multicast_ip_;
    int         multicast_port_ = 0;
    int         interval_ms_ = 3000;

    std::thread worker_;
    std::atomic<bool> running_{false};

    int sock_ = -1;
};

} // namespace utils
} // namespace yw


