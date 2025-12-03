#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
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

        // 根据IP获取单个节点（含元数据）
        virtual std::optional<NodeExt> getNodeByIP(const std::string& ip) const = 0;

        // 根据机箱号获取该机箱号下的所有节点（含元数据）
        virtual std::vector<NodeExt> getNodesByBoxId(int box_id) const = 0;

        /**
         * @brief 设置告警回调函数
         * @param callback 回调函数，参数为：box_id、slot_id、缓存的板卡类型、新的板卡类型
         */
        virtual void setAlertCallback(std::function<void(int, int, const std::string&, const std::string&)> callback) = 0;
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