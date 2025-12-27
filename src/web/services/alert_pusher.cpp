#include "alert_pusher.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <hv/WebSocketServer.h>
#include <hv/WebSocketChannel.h>

namespace yw {
namespace web {

// 告警推送器构造函数
// server: HTTP服务器实例，用于注册WebSocket服务
AlertPusher::AlertPusher(hv::HttpServer* server)
    : server_(server),
      ws_service_(std::make_unique<hv::WebSocketService>()) {

        server_->ws = ws_service_.get();
        init();
    }

// 初始化WebSocket服务，注册连接、断开和消息处理回调
// 返回: 初始化成功返回true
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

// 向所有连接的WebSocket客户端推送告警JSON消息
// alertJson: 要推送的告警JSON对象
// 自动清理已断开的连接
void AlertPusher::pushJson(const nlohmann::json& alertJson) {
    std::string payload = alertJson.dump();
    
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

// 停止告警推送器，清理所有连接和WebSocket服务
void AlertPusher::stop() {
    std::lock_guard<std::mutex> lk(mu_);
    if (server_) {
        server_->ws = nullptr;
    }
    channels_.clear();
    ws_service_.reset();
}

} // namespace web
} // namespace yw
