#include "AlertPusher.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <hv/WebSocketServer.h>
#include <hv/WebSocketChannel.h>
#include "../../alertv2/domain/Alert.h"
#include "dto/alert_dto.h"

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

void AlertPusher::pushV2(const alertv2::Alert& alert) {
    // 将 alertv2::Alert 转换为 UserAlertEventView
    web::UserAlertEventView view;
    
    // 设置基本信息
    view.fingerprint = alert.getFingerprint();
    view.id = alert.getId();
    
    // 将 AlertStatus 枚举转换为字符串
    std::string statusStr;
    switch (alert.getStatus()) {
        case alertv2::AlertStatus::Pending:
            statusStr = "pending";
            break;
        case alertv2::AlertStatus::Firing:
            statusStr = "firing";
            break;
        case alertv2::AlertStatus::Resolved:
            statusStr = "resolved";
            break;
        default:
            statusStr = "unknown";
            break;
    }
    view.status = statusStr;
    
    view.created_at = alert.getCreatedAt();
    view.starts_at = alert.getStartsAt();
    view.ends_at = alert.getEndsAt();
    view.updated_at = alert.getUpdatedAt();
    
    // 设置标签
    const auto& alertLabels = alert.getLabels();
    view.labels = std::map<std::string, std::string>(alertLabels.begin(), alertLabels.end());
    
    // 设置注释
    view.annotations.description = alert.getAnnotations().count("description") ? 
                                  alert.getAnnotations().at("description") : "";
    view.annotations.summary = alert.getAnnotations().count("summary") ? 
                              alert.getAnnotations().at("summary") : "";
    
    // 序列化为 JSON 并推送
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
    if (server_) {
        server_->ws = nullptr;
    }
    channels_.clear();
    ws_service_.reset();
}

} // namespace web
} // namespace yw
