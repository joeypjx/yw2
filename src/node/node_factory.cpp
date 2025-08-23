#include "../../include/yw/node.h"
#include "node_manager.h"
#include <hv/HttpService.h>
#include <mutex>

namespace yw {
namespace node {

std::shared_ptr<INodeModule> NodeFactory::getNodeModule(std::shared_ptr<hv::HttpService> service) {
    static std::shared_ptr<INodeModule> instance;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        instance = std::make_shared<NodeManager>(service);
    }
    return instance;
}

} // namespace node
} // namespace yw
