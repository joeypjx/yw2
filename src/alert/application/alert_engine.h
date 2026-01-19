// ============================================================================
// 文件功能描述：
// 告警引擎（AlertEngine）的头文件，定义告警规则评估引擎的接口和实现。
// 主要功能包括：
// 1. 规则评估：定期执行所有启用的告警规则，评估条件是否满足
// 2. 告警生成：当规则条件满足时，创建或更新告警事件
// 3. 状态管理：管理告警事件的状态转换（Pending -> Firing -> Resolved）
// 4. 持续时间检查：检查告警是否持续满足条件达到指定的持续时间（for字段）
// 5. 回调通知：当告警状态变化时，调用推送回调函数通知外部系统
// 6. 统计信息：记录评估次数、生成的告警数量等统计信息
// 7. 线程管理：使用独立线程运行评估循环，支持优雅启动和停止
// ============================================================================

#pragma once

#include "../domain/alert_rule.h"
#include "../domain/alert_event.h"
#include "../infrastructure/alert_event_repository.h"
#include "../infrastructure/database_query_interface.h"
#include "alert_creation_factory.h"
#include "alert_rule_service.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_set>
#include <functional>

namespace yw {
namespace alert {

/**
 * @brief 告警引擎类
 * 
 * 负责定期评估告警规则，管理告警状态，并更新数据库
 */
// 告警引擎类，负责定期评估告警规则，管理告警状态，并更新数据库
class AlertEngine {
public:
    // 构造函数，初始化告警引擎
    // dbInterface: 数据库查询接口
    // alertRepo: 告警事件仓库
    // alertRuleService: 告警规则服务
    // alertFactory: 告警创建工厂（可选，如果不提供则内部创建）
    AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                std::shared_ptr<AlertEventRepository> alertRepo,
                std::shared_ptr<AlertRuleService> alertRuleService,
                std::shared_ptr<AlertCreationFactory> alertFactory = nullptr);
    // 析构函数，停止告警引擎
    ~AlertEngine();
    // 启动告警引擎，开始定期评估告警规则
    // intervalSeconds: 评估间隔（秒），默认5秒
    void start(int intervalSeconds = 5);
    // 停止告警引擎
    void stop();
    // 设置告警推送回调函数
    // callback: 当创建或更新告警时调用的回调函数
    void setPushCallback(std::function<void(const AlertEvent&)> callback);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    std::shared_ptr<AlertEventRepository> alertRepo_;
    
    // 服务类实例
    std::shared_ptr<AlertCreationFactory> alertFactory_;
    std::shared_ptr<AlertRuleService> alertRuleService_;
    
    // 运行状态
    std::atomic<bool> running_;
    std::atomic<bool> shouldStop_;
    std::thread workerThread_;
    
    // 评估间隔
    int intervalSeconds_;
    
    std::function<void(const AlertEvent&)> pushCallback_;
    
    // 工作线程主循环
    void workerLoop();
    // 初始化告警引擎，创建必要的服务实例
    void initialize();
    // 执行一次完整的告警评估
    // 返回: 本次评估生成的告警数量
    int performEvaluation();
    // 评估所有启用的告警规则
    // 返回: 生成的告警事件列表
    std::vector<AlertEvent> evaluateAllRules();
    // 处理告警状态更新（Pending转Firing，Firing转Resolved）
    // currentAlerts: 当前评估生成的告警列表
    // 返回: 状态更新的告警数量
    int processAlertStatusUpdates(const std::vector<AlertEvent>& currentAlerts);
    // 将告警列表更新到数据库
    // alerts: 要更新的告警列表
    // 返回: 成功更新的告警数量
    int updateAlertsToDatabase(const std::vector<AlertEvent>& alerts);
    // 获取当前所有Firing状态的告警指纹集合
    std::unordered_set<std::string> getCurrentFiringFingerprints();
    // 获取当前所有Pending状态的告警指纹集合
    std::unordered_set<std::string> getCurrentPendingFingerprints();
    // 判断Pending告警是否应该转为Firing状态
    // pendingAlert: 待判断的Pending告警
    // 返回: 应该转为Firing返回true，否则返回false
    bool shouldTransitionToFiring(const AlertEvent& pendingAlert);
    // 检查告警对应的节点是否有最近的数据
    // alert: 告警事件
    // seconds: 时间窗口（秒），默认10秒
    // 返回: 有最近数据返回true，否则返回false
    bool hasNodeRecentData(const AlertEvent& alert, int seconds = 10);
};

} // namespace alert
} // namespace yw
