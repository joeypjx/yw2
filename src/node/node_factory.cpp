// ============================================================================
// 文件功能描述：
// 节点工厂（NodeFactory）的实现文件，负责创建和初始化节点模块实例。
// 主要功能包括：
// 1. 模块创建：创建NodeManager实例，传入HTTP服务用于注册API路由
// 2. 工厂模式：提供统一的模块创建接口，隐藏具体的实现细节
// 3. 依赖注入：接收HTTP服务实例，实现模块间的松耦合
// ============================================================================

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
