#pragma once

#include "../domain/alert_rule.h"
#include "../domain/Alert.h"
#include "../infrastructure/alert_rule_repository.h"
#include "../infrastructure/alert_repository.h"
#include "../infrastructure/database_query_interface.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_set>
#include <functional>

namespace yw {
namespace node {
    class INodeModule;  // 前向声明
}

namespace alert {

/**
 * @brief 告警引擎类
 * 
 * 负责定期评估告警规则，管理告警状态，并更新数据库
 */
class AlertEngine {
public:
    AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                std::shared_ptr<AlertRuleRepository> alertRuleRepo,
                std::shared_ptr<AlertRepository> alertRepo,
                node::INodeModule* nodeModule = nullptr);
    ~AlertEngine();
    void start(int intervalSeconds = 5);
    void stop();
    
    bool addAlertRule(const AlertRule& rule);
    bool updateAlertRule(const AlertRule& rule);
    bool deleteAlertRule(const std::string& ruleId);
    std::shared_ptr<AlertRule> getAlertRuleById(const std::string& ruleId);
    std::vector<AlertRule> getAllAlertRules() const;
    
    std::vector<Alert> getAlertsByStatus(const std::string& status);
    std::vector<Alert> getAlertsByHostIp(const std::string& hostIp);
    std::vector<Alert> getAlertsByBoxId(int boxId);
    std::vector<Alert> getAlertsBySlotId(int slotId);
    std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime);
    std::vector<Alert> getAlertsByAlertType(const std::string& alertType);
    std::vector<Alert> getAlertsBySeverity(const std::string& severity);
    std::vector<Alert> getAlertsByDescription(const std::string& description);
    std::vector<Alert> getAlertsExceptPending();
    std::vector<Alert> getAlertsByFilters(const AlertFilters& filters);
    std::shared_ptr<Alert> getAlertById(const std::string& alertId);
    
    std::shared_ptr<Alert> createAlertFromComponent(const std::string& hostIp,
                                                   const std::string& instanceId,
                                                   const std::string& uuid,
                                                   int index,
                                                   const std::string& status,
                                                   const std::string& stack_name,
                                                   const std::string& component_name);
    
    /**
     * @brief 创建节点板卡类型变化告警
     * @param box_id 机箱ID
     * @param slot_id 槽位ID
     * @param cached_board_type 缓存的板卡类型
     * @param new_board_type 新的板卡类型
     * @return 创建的告警对象，失败返回nullptr
     */
    std::shared_ptr<Alert> createBoardTypeChangeAlert(int box_id, int slot_id, 
                                                      const std::string& cached_board_type, 
                                                      const std::string& new_board_type);
    
    size_t getAlertCount();
    void setPushCallback(std::function<void(const Alert&)> callback);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    std::shared_ptr<AlertRuleRepository> alertRuleRepo_;
    std::shared_ptr<AlertRepository> alertRepo_;
    node::INodeModule* nodeModule_;  // 可选的 node 模块，用于获取 box_id 和 slot_id
    
    // 内存中的告警规则
    std::vector<AlertRule> rules_;
    
    // 运行状态
    std::atomic<bool> running_;
    std::atomic<bool> shouldStop_;
    std::thread workerThread_;
    
    // 评估间隔
    int intervalSeconds_;
    
    // 节点心跳超时阈值（秒）
    int heartbeatTimeoutSeconds_;
    
    // 统计信息
    std::atomic<int> totalEvaluations_;
    std::atomic<int> totalAlertsGenerated_;
    std::chrono::system_clock::time_point lastEvaluationTime_;
    std::chrono::system_clock::time_point startTime_;
    
    std::function<void(const Alert&)> pushCallback_;
    
    void workerLoop();
    void initialize();
    int performEvaluation();
    int performEvaluationForAlive();
    std::vector<Alert> evaluateAllRules();
    int processAlertStatusUpdates(const std::vector<Alert>& currentAlerts);
    int updateAlertsToDatabase(const std::vector<Alert>& alerts);
    std::unordered_set<std::string> getCurrentFiringFingerprints();
    std::unordered_set<std::string> getCurrentPendingFingerprints();
    std::string generateAlertId();
    bool shouldTransitionToFiring(const Alert& pendingAlert);
    int parseDuration(const std::string& duration);
    std::chrono::system_clock::time_point parseISOTime(const std::string& isoTime);
    bool hasNodeRecentData(const Alert& alert, int seconds = 10);
};

} // namespace alert
} // namespace yw
