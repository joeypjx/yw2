#pragma once

#include <memory>
#include <string>

// 前向声明
namespace hv {
    class HttpServer;
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
        static std::shared_ptr<INodeModule> createNodeModule(std::shared_ptr<hv::HttpServer> server);
    };

} // namespace node
} // namespace yw