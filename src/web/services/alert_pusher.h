// ============================================================================
// 文件功能描述：
// 告警推送器（AlertPusher）的头文件，定义告警事件WebSocket实时推送功能的接口。
// 主要功能包括：
// 1. WebSocket服务管理：注册WebSocket服务到HTTP服务器，处理客户端连接
// 2. 连接管理：维护所有连接的WebSocket客户端列表，自动清理断开的连接
// 3. 消息推送：向所有连接的客户端推送告警JSON消息
// 4. 生命周期管理：在析构时自动停止WebSocket服务并清理所有连接
// 5. 线程安全：使用互斥锁保护连接列表，支持多线程并发访问
// ============================================================================

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
