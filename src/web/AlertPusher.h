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

#include "yw/alert_model.h"
#include <hv/WebSocketServer.h>
#include <hv/WebSocketChannel.h>

namespace yw {
namespace web {

class AlertPusher {
public:
    AlertPusher();

    bool start(const char* ip_port = ":8081");

    void push(const alert::AlertEvent& event);

private:
    std::unique_ptr<hv::WebSocketServer>                   server_;
    std::unique_ptr<hv::WebSocketService>                  ws_service_;
    
    std::mutex                                              mu_;
    std::unordered_set<std::shared_ptr<hv::WebSocketChannel>> channels_;
    bool                                                    started_ {false};
};

} // namespace web
} // namespace yw
