#pragma once

#include "../domain/AlertRule.h"
#include "../domain/Alert.h"
#include "../infrastructure/AlertRuleRepository.h"
#include "../infrastructure/AlertRepository.h"
#include "../infrastructure/DatabaseQueryInterface.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_set>
#include <functional>

namespace yw {
namespace alertv2 {

/**
 * @brief 告警引擎类
 * 
 * 负责定期评估告警规则，管理告警状态，并更新数据库
 */
class AlertEngine {
public:
    AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                std::shared_ptr<AlertRuleRepository> alertRuleRepo,
                std::shared_ptr<AlertRepository> alertRepo);
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
    std::vector<Alert> getAlertsByAlertType(const std::string& alertType);
    std::vector<Alert> getAlertsBySeverity(const std::string& severity);
    std::vector<Alert> getAlertsExceptPending();
    std::shared_ptr<Alert> getAlertById(const std::string& alertId);
    
    std::shared_ptr<Alert> createAlertFromComponent(const std::string& hostIp,
                                                   const std::string& instanceId,
                                                   const std::string& uuid,
                                                   const std::string& index,
                                                   const std::string& status);
    
    size_t getAlertCount();
    void setPushCallback(std::function<void(const Alert&)> callback);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    std::shared_ptr<AlertRuleRepository> alertRuleRepo_;
    std::shared_ptr<AlertRepository> alertRepo_;
    
    // 内存中的告警规则
    std::vector<AlertRule> rules_;
    
    // 运行状态
    std::atomic<bool> running_;
    std::atomic<bool> shouldStop_;
    std::thread workerThread_;
    
    // 评估间隔
    int intervalSeconds_;
    
    // 统计信息
    std::atomic<int> totalEvaluations_;
    std::atomic<int> totalAlertsGenerated_;
    std::chrono::system_clock::time_point lastEvaluationTime_;
    std::chrono::system_clock::time_point startTime_;
    
    std::function<void(const Alert&)> pushCallback_;
    
    void workerLoop();
    void initialize();
    int performEvaluation();
    std::vector<Alert> evaluateAllRules();
    int processAlertStatusUpdates(const std::vector<Alert>& currentAlerts);
    int updateAlertsToDatabase(const std::vector<Alert>& alerts);
    std::unordered_set<std::string> getCurrentFiringFingerprints();
    std::unordered_set<std::string> getCurrentPendingFingerprints();
    std::string generateAlertId();
    bool shouldTransitionToFiring(const Alert& pendingAlert);
    int parseDuration(const std::string& duration);
    std::chrono::system_clock::time_point parseISOTime(const std::string& isoTime);
};

} // namespace alertv2
} // namespace yw
