#include "AlertPusher.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <hv/WebSocketServer.h>
#include <hv/WebSocketChannel.h>
#include "alert_view_utils.h"

namespace yw {
namespace web {

AlertPusher::AlertPusher(hv::HttpServer* server)
    : server_(server),
      ws_service_(std::make_unique<hv::WebSocketService>()) {

        server_->ws = ws_service_.get();
        init();
    }

bool AlertPusher::init() {
    std::lock_guard<std::mutex> lk(mu_);

    ws_service_->onopen = [this](const WebSocketChannelPtr& channel, const HttpRequestPtr&){
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

    return true;
}

void AlertPusher::push(const alert::AlertEvent& event) {
    auto view = toUserAlertEventView(event);
    nlohmann::json j = view;
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

void AlertPusher::stop() {
    std::lock_guard<std::mutex> lk(mu_);
    channels_.clear();
    ws_service_.reset();
}

} // namespace web
} // namespace yw
