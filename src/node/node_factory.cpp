#include "node/node.h"
#include "node_manager.h"
#include <hv/HttpService.h>
#include <mutex>

namespace yw {
namespace node {

std::shared_ptr<INodeModule> NodeFactory::getNodeModule(std::shared_ptr<hv::HttpService> service) {
    return std::make_shared<NodeManager>(service);
}

} // namespace node
} // namespace yw
