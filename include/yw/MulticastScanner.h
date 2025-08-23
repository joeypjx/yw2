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


