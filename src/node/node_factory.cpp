#include "node/node.h"
#include "node_manager.h"
#include <hv/HttpService.h>
#include <mutex>

namespace yw {
namespace node {

// 创建节点模块实例
// service: HTTP服务实例，用于注册节点相关的API路由
// 返回: 节点模块共享指针
std::shared_ptr<INodeModule> NodeFactory::getNodeModule(std::shared_ptr<hv::HttpService> service) {
    return std::make_shared<NodeManager>(service);
}

} // namespace node
} // namespace yw
