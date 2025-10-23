#ifndef ALERT_RULE_TESTER_H
#define ALERT_RULE_TESTER_H

#include "../domain/AlertRule.h"
#include "../infrastructure/AlertRuleRepository.h"
#include "../infrastructure/AlertRepository.h"
#include "../infrastructure/DatabaseQueryInterface.h"
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

namespace yw {
namespace alertv2 {

/**
 * @brief 告警规则测试器
 * 
 * 用于从数据库中获取告警规则，循环运行evaluate方法，
 * 测试告警规则评估功能是否正常工作
 */
class AlertRuleTester {
public:
    /**
     * @brief 构造函数
     * @param dbInterface 数据库查询接口
     * @param alertRepo 告警存储接口（可选）
     */
    explicit AlertRuleTester(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                            std::shared_ptr<AlertRepository> alertRepo = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~AlertRuleTester() = default;
    
    /**
     * @brief 运行测试
     * @param intervalSeconds 测试间隔时间（秒）
     * @param maxRounds 最大测试轮数，0表示无限循环
     */
    void runTest(int intervalSeconds = 30, int maxRounds = 0);
    
    /**
     * @brief 运行单次测试
     * @return 生成的告警数量
     */
    int runSingleTest();
    
    /**
     * @brief 停止测试
     */
    void stopTest();

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    std::shared_ptr<DatabaseAlertRuleRepository> repository_;
    std::shared_ptr<AlertRepository> alertRepo_;
    bool shouldStop_;
    
    /**
     * @brief 打印告警规则信息
     * @param rules 告警规则列表
     */
    void printRules(const std::vector<AlertRule>& rules);
    
    /**
     * @brief 打印告警信息
     * @param alerts 告警列表
     */
    void printAlerts(const std::vector<Alert>& alerts);
};

} // namespace alertv2
} // namespace yw

#endif // ALERT_RULE_TESTER_H
