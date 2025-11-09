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

// Forward declaration for AlertV2
namespace yw {
namespace alert {
    class Alert;
}
}

namespace yw {
namespace web {

class AlertPusher {
public:
    AlertPusher(hv::HttpServer* server);

    void pushV2(const alert::Alert& alert);

    void stop();

private:
    bool init();

    hv::HttpServer*                                        server_;
    std::unique_ptr<hv::WebSocketService>                  ws_service_;
    
    std::mutex                                              mu_;
    std::unordered_set<std::shared_ptr<hv::WebSocketChannel>> channels_;
};

} // namespace web
} // namespace yw
