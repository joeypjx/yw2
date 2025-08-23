#pragma once

#include <memory>
#include <string>
#include <vector>
#include "yw/node_model.h"

// 前向声明
namespace hv {
    class HttpServer;
    class HttpService;
}

namespace yw {
namespace node {

    /**
     * @brief 节点管理模块接口
     * 
     * 提供节点管理的公共接口，隐藏内部实现细节
     */
    class INodeModule {
    public:
        virtual ~INodeModule() = default;

        // 获取所有节点（含元数据）
        virtual std::vector<NodeExt> getAllNodes() const = 0;
    };

    /**
     * @brief 节点管理模块工厂
     * 
     * 负责创建节点管理模块实例
     */
    class NodeFactory {
    public:
        /**
         * @brief 创建节点管理模块
         * @param server HTTP服务器实例
         * @return 节点管理模块智能指针
         */
        static std::shared_ptr<INodeModule> getNodeModule(std::shared_ptr<hv::HttpService> service);
    };

} // namespace node
} // namespace yw