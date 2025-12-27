// ============================================================================
// 文件功能描述：
// 节点模块（INodeModule）的头文件，定义节点管理模块的公共接口。
// 主要功能包括：
// 1. 节点查询接口：提供查询所有节点、按IP查询、按机箱号查询等接口
// 2. 告警回调：提供设置板卡类型变化告警回调的接口
// 3. 工厂模式：提供NodeFactory工厂类，用于创建节点模块实例
// 4. 接口抽象：定义INodeModule接口，隐藏具体实现细节，实现模块间的松耦合
// ============================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include "node/node_model.h"

// 前向声明
namespace hv {
    class HttpServer;
    struct HttpService;
}

namespace yw {
namespace node {

    /**
     * @brief 节点管理模块接口
     * 
     * 提供节点管理的公共接口，隐藏内部实现细节。
     * 支持节点查询、状态管理和告警回调等功能。
     */
    class INodeModule {
    public:
        virtual ~INodeModule() = default;

        // ========== 节点查询 ==========

        /**
         * @brief 获取所有节点（含元数据）
         * 
         * @return 所有节点的扩展信息列表，包含节点基本信息和状态信息
         */
        virtual std::vector<NodeExt> getAllNodes() const = 0;

        /**
         * @brief 根据IP地址获取单个节点（含元数据）
         * 
         * @param ip 节点的IP地址
         * @return 节点的扩展信息，如果节点不存在返回 std::nullopt
         */
        virtual std::optional<NodeExt> getNodeByIP(const std::string& ip) const = 0;

        /**
         * @brief 根据机箱号获取该机箱下的所有节点（含元数据）
         * 
         * @param box_id 机箱号（通常范围 1-9）
         * @return 指定机箱下所有节点的扩展信息列表
         */
        virtual std::vector<NodeExt> getNodesByBoxId(int box_id) const = 0;

        // ========== 回调设置 ==========

        /**
         * @brief 设置告警回调函数
         * 
         * 当节点板卡类型发生变化时，会触发此回调函数。
         * 
         * @param callback 回调函数，参数依次为：
         *   - box_id: 机箱号
         *   - slot_id: 板卡槽位号
         *   - cached_board_type: 缓存的板卡类型
         *   - new_board_type: 新的板卡类型
         */
        virtual void setAlertCallback(std::function<void(int, int, const std::string&, const std::string&)> callback) = 0;
    };

    /**
     * @brief 节点管理模块工厂类
     * 
     * 负责创建和获取节点管理模块实例。
     */
    class NodeFactory {
    public:
        /**
         * @brief 创建节点管理模块实例
         * 
         * @param service HTTP服务实例，用于节点通信
         * @return 节点管理模块的共享指针，如果创建失败可能返回 nullptr
         */
        static std::shared_ptr<INodeModule> getNodeModule(std::shared_ptr<hv::HttpService> service);
    };

} // namespace node
} // namespace yw