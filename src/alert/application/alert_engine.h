#pragma once

#include "../domain/alert_rule.h"
#include "../domain/alert_event.h"
#include "../infrastructure/alert_rule_repository.h"
#include "../infrastructure/alert_event_repository.h"
#include "../infrastructure/database_query_interface.h"
#include "alert_creation_factory.h"
#include "alert_rule_service.h"
#include "alert_query_service.h"
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
class AlertEngine {
public:
    AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                std::shared_ptr<AlertRuleRepository> alertRuleRepo,
                std::shared_ptr<AlertEventRepository> alertRepo);
    ~AlertEngine();
    void start(int intervalSeconds = 5);
    void stop();
    void setPushCallback(std::function<void(const AlertEvent&)> callback);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    std::shared_ptr<AlertRuleRepository> alertRuleRepo_;
    std::shared_ptr<AlertEventRepository> alertRepo_;
    
    // 服务类实例
    std::shared_ptr<AlertCreationFactory> alertFactory_;
    std::shared_ptr<AlertRuleService> alertRuleService_;
    std::shared_ptr<AlertQueryService> alertQueryService_;
    
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
    
    std::function<void(const AlertEvent&)> pushCallback_;
    
    void workerLoop();
    void initialize();
    int performEvaluation();
    std::vector<AlertEvent> evaluateAllRules();
    int processAlertStatusUpdates(const std::vector<AlertEvent>& currentAlerts);
    int updateAlertsToDatabase(const std::vector<AlertEvent>& alerts);
    std::unordered_set<std::string> getCurrentFiringFingerprints();
    std::unordered_set<std::string> getCurrentPendingFingerprints();
    bool shouldTransitionToFiring(const AlertEvent& pendingAlert);
    bool hasNodeRecentData(const AlertEvent& alert, int seconds = 10);
};

} // namespace alert
} // namespace yw
