#pragma once

#include <memory>
#include <mutex>
#include <unordered_set>

#ifdef HAVE_STDINT_H
#undef HAVE_STDINT_H
#endif

#ifdef HAVE_SYS_TYPES_H
#undef HAVE_SYS_TYPES_H
#endif

#ifdef HAVE_SYS_STAT_H
#undef HAVE_SYS_STAT_H
#endif

#include <hv/HttpServer.h>
#include <hv/WebSocketServer.h>
#include <hv/WebSocketChannel.h>

#include <nlohmann/json.hpp>

namespace yw {
namespace web {

// 告警推送器，负责通过WebSocket向客户端推送告警消息
class AlertPusher {
public:
    // 构造函数，初始化WebSocket服务
    // server: HTTP服务器实例
    AlertPusher(hv::HttpServer* server);

    // 推送告警JSON到所有WebSocket客户端
    // alertJson: 告警的JSON对象
    // 自动清理已断开的连接
    void pushJson(const nlohmann::json& alertJson);

    // 停止告警推送器，清理所有连接和WebSocket服务
    void stop();

private:
    // 初始化WebSocket服务，注册连接、断开和消息处理回调
    // 返回: 初始化成功返回true
    bool init();

    hv::HttpServer*                                        server_;
    std::unique_ptr<hv::WebSocketService>                  ws_service_;
    
    std::mutex                                              mu_;
    std::unordered_set<std::shared_ptr<hv::WebSocketChannel>> channels_;
};

} // namespace web
} // namespace yw
