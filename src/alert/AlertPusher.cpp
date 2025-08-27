#include "AlertPusher.h"
#include <string>
#include <memory>
#include "yw/alert_model.h"
#include <nlohmann/json.hpp>
#include <hv/WebSocketServer.h>
#include <hv/WebSocketChannel.h>

namespace yw {
namespace alert {

AlertPusher::AlertPusher()
    : server_(std::make_unique<hv::WebSocketServer>()),
      ws_service_(std::make_unique<hv::WebSocketService>()) {}

bool AlertPusher::start(const char* ip_port) {
    std::lock_guard<std::mutex> lk(mu_);
    if (started_) return true;

    ws_service_->onopen = [this](const WebSocketChannelPtr& channel, const HttpRequestPtr& req){
        (void)req;
        std::lock_guard<std::mutex> g(mu_);
        channels_.insert(channel);
    };
    ws_service_->onclose = [this](const WebSocketChannelPtr& channel){
        std::lock_guard<std::mutex> g(mu_);
        channels_.erase(channel);
    };
    ws_service_->onmessage = [](const WebSocketChannelPtr& channel, const std::string& msg){
        channel->send(msg);
    };

    server_->registerWebSocketService(ws_service_.get());
    server_->run(ip_port, false);
    started_ = true;
    return true;
}

void AlertPusher::push(const AlertEvent& event) {
    nlohmann::json j = event;
    std::string payload = j.dump();
    std::lock_guard<std::mutex> lk(mu_);
    for (auto it = channels_.begin(); it != channels_.end(); ) {
        auto ch = *it;
        if (!ch || !ch->isConnected()) {
            it = channels_.erase(it);
            continue;
        }
        ch->send(payload);
        ++it;
    }
}

} // namespace alert
} // namespace yw


