#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "alert/alert_model.h"

namespace yw {
namespace node { class INodeModule; }
namespace alert {

// 前向声明内部实现类
class Alert;
class AlertRule;
class AlertEngine;

//=============================================================================
// 告警模块接口
//=============================================================================

/**
 * @brief 告警模块接口
 * 
 * 提供告警管理的公共接口，隐藏内部实现细节
 */
class IAlertModule {
public:
    virtual ~IAlertModule() = default;

    //-------------------------------------------------------------------------
    // 生命周期管理
    //-------------------------------------------------------------------------
    
    /**
     * @brief 启动告警引擎
     * @param intervalSeconds 评估间隔（秒），默认5秒
     */
    virtual void start(int intervalSeconds = 5) = 0;
    
    /**
     * @brief 停止告警引擎
     */
    virtual void stop() = 0;
    
    /**
     * @brief 检查告警引擎是否正在运行
     * @return 是否正在运行
     */
    virtual bool isRunning() const = 0;

    //-------------------------------------------------------------------------
    // 告警规则管理
    //-------------------------------------------------------------------------
    
    /**
     * @brief 添加告警规则
     * @param ruleJson 告警规则的JSON表示
     * @return 成功返回true，失败返回false
     */
    virtual bool addAlertRule(const nlohmann::json& ruleJson) = 0;
    
    /**
     * @brief 更新告警规则
     * @param ruleJson 告警规则的JSON表示
     * @return 成功返回true，失败返回false
     */
    virtual bool updateAlertRule(const nlohmann::json& ruleJson) = 0;
    
    /**
     * @brief 删除告警规则
     * @param ruleId 规则ID
     * @return 成功返回true，失败返回false
     */
    virtual bool deleteAlertRule(const std::string& ruleId) = 0;
    
    /**
     * @brief 根据ID获取告警规则
     * @param ruleId 规则ID
     * @return 规则的JSON表示，未找到返回空JSON
     */
    virtual nlohmann::json getAlertRuleById(const std::string& ruleId) = 0;
    
    /**
     * @brief 获取所有告警规则
     * @return 所有规则的JSON数组
     */
    virtual nlohmann::json getAllAlertRules() const = 0;

    //-------------------------------------------------------------------------
    // 告警查询
    //-------------------------------------------------------------------------
    
    /**
     * @brief 根据状态获取告警
     * @param status 告警状态 (pending/firing/resolved)
     * @return 告警列表的JSON数组
     */
    virtual nlohmann::json getAlertsByStatus(const std::string& status) = 0;
    
    /**
     * @brief 根据多条件过滤获取告警
     * @param filters 过滤条件
     * @return 告警列表的JSON数组
     */
    virtual nlohmann::json getAlertsByFilters(const AlertFilters& filters) = 0;
    
    /**
     * @brief 根据ID获取告警
     * @param alertId 告警ID
     * @return 告警的JSON表示，未找到返回空JSON
     */
    virtual nlohmann::json getAlertById(const std::string& alertId) = 0;
    
    /**
     * @brief 获取除 pending 状态外的所有告警
     * @return 告警列表的JSON数组
     */
    virtual nlohmann::json getAlertsExceptPending() = 0;

    //-------------------------------------------------------------------------
    // 统计信息
    //-------------------------------------------------------------------------
    
    /**
     * @brief 获取告警总数
     * @return 告警总数
     */
    virtual size_t getAlertCount() = 0;

    //-------------------------------------------------------------------------
    // 回调设置
    //-------------------------------------------------------------------------
    
    /**
     * @brief 设置告警推送回调
     * @param callback 回调函数，参数为告警的JSON表示
     */
    virtual void setPushCallback(std::function<void(const nlohmann::json&)> callback) = 0;
    
    //-------------------------------------------------------------------------
    // 告警创建
    //-------------------------------------------------------------------------
    
    /**
     * @brief 创建节点板卡类型变化告警
     * @param box_id 机箱ID
     * @param slot_id 槽位ID
     * @param cached_board_type 缓存的板卡类型
     * @param new_board_type 新的板卡类型
     * @return 创建的告警JSON，失败返回空JSON
     */
    virtual nlohmann::json createBoardTypeChangeAlert(
        int box_id, int slot_id,
        const std::string& cached_board_type,
        const std::string& new_board_type) = 0;
};

//=============================================================================
// 告警模块工厂
//=============================================================================

/**
 * @brief 告警模块工厂
 * 
 * 负责创建告警模块实例
 */
class AlertFactory {
public:
    /**
     * @brief 创建告警模块
     * @param dbConnInfo 数据库连接字符串
     * @param nodeModule 节点模块指针（可选，用于获取节点信息）
     * @return 告警模块智能指针，失败返回nullptr
     */
    static std::shared_ptr<IAlertModule> createAlertModule(
        const std::string& dbConnInfo,
        node::INodeModule* nodeModule = nullptr);
};

} // namespace alert
} // namespace yw

