#include "../../include/yw/node.h"
#include "node_manager.h"

namespace yw {
namespace node {

std::shared_ptr<INodeModule> NodeFactory::createNodeModule(std::shared_ptr<hv::HttpServer> server) {
    return std::make_shared<NodeManager>(server);
}

} // namespace node
} // namespace yw
