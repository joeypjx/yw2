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

namespace yw {
namespace alertv2 {

/**
 * @brief 告警引擎类
 * 
 * 负责定期评估告警规则，管理告警状态，并更新数据库
 */
class AlertEngine {
public:
    /**
     * @brief 构造函数
     * @param dbInterface 数据库查询接口
     * @param alertRuleRepo 告警规则存储接口
     * @param alertRepo 告警存储接口
     */
    AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                std::shared_ptr<AlertRuleRepository> alertRuleRepo,
                std::shared_ptr<AlertRepository> alertRepo);
    
    /**
     * @brief 析构函数
     */
    ~AlertEngine();
    
    /**
     * @brief 启动告警引擎
     * @param intervalSeconds 评估间隔时间（秒），默认5秒
     */
    void start(int intervalSeconds = 5);
    
    /**
     * @brief 停止告警引擎
     */
    void stop();
    
    /**
     * @brief 检查引擎是否正在运行
     * @return true如果正在运行，false否则
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief 手动触发一次评估
     * @return 本次评估生成的告警数量
     */
    int evaluateOnce();
    
    /**
     * @brief 重新加载告警规则
     */
    void reloadRules();
    
    /**
     * @brief 获取当前内存中的告警规则数量
     * @return 告警规则数量
     */
    size_t getRuleCount() const { return rules_.size(); }
    
    /**
     * @brief 获取上次评估时间
     * @return 上次评估时间
     */
    std::chrono::system_clock::time_point getLastEvaluationTime() const { return lastEvaluationTime_; }
    
    /**
     * @brief 获取评估统计信息
     * @return 评估统计信息字符串
     */
    std::string getStatistics() const;
    
    // 告警规则管理方法
    /**
     * @brief 添加新的告警规则
     * @param rule 要添加的告警规则
     * @return 是否添加成功
     */
    bool addAlertRule(const AlertRule& rule);
    
    /**
     * @brief 更新现有的告警规则
     * @param rule 要更新的告警规则
     * @return 是否更新成功
     */
    bool updateAlertRule(const AlertRule& rule);
    
    /**
     * @brief 删除告警规则
     * @param ruleId 要删除的告警规则ID
     * @return 是否删除成功
     */
    bool deleteAlertRule(const std::string& ruleId);
    
    /**
     * @brief 根据ID获取告警规则
     * @param ruleId 告警规则ID
     * @return 告警规则对象，如果不存在则返回nullptr
     */
    std::shared_ptr<AlertRule> getAlertRuleById(const std::string& ruleId);
    
    /**
     * @brief 获取所有告警规则
     * @return 告警规则列表
     */
    std::vector<AlertRule> getAllAlertRules() const;
    
    /**
     * @brief 检查告警规则是否存在
     * @param ruleId 告警规则ID
     * @return 是否存在
     */
    bool alertRuleExists(const std::string& ruleId) const;
    
    /**
     * @brief 同步数据库中的告警规则到内存
     * @return 同步的规则数量
     */
    int syncRulesFromDatabase();
    
    // 告警查询方法
    /**
     * @brief 根据状态获取告警列表
     * @param status 告警状态
     * @return 告警列表
     */
    std::vector<Alert> getAlertsByStatus(const std::string& status);
    
    /**
     * @brief 根据主机IP获取告警列表
     * @param hostIp 主机IP地址
     * @return 告警列表
     */
    std::vector<Alert> getAlertsByHostIp(const std::string& hostIp);
    
    /**
     * @brief 根据告警规则ID获取告警列表
     * @param ruleId 告警规则ID
     * @return 告警列表
     */
    std::vector<Alert> getAlertsByRuleId(const std::string& ruleId);
    
    /**
     * @brief 根据告警名称获取告警列表
     * @param alertName 告警名称
     * @return 告警列表
     */
    std::vector<Alert> getAlertsByAlertName(const std::string& alertName);
    
    /**
     * @brief 根据告警类型获取告警列表
     * @param alertType 告警类型
     * @return 告警列表
     */
    std::vector<Alert> getAlertsByAlertType(const std::string& alertType);
    
    /**
     * @brief 根据严重程度获取告警列表
     * @param severity 严重程度
     * @return 告警列表
     */
    std::vector<Alert> getAlertsBySeverity(const std::string& severity);
    
    /**
     * @brief 根据时间范围获取告警列表
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @return 告警列表
     */
    std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime);
    
    /**
     * @brief 获取最近的告警列表
     * @param limit 限制数量，默认100
     * @return 告警列表
     */
    std::vector<Alert> getRecentAlerts(int limit = 100);
    
    /**
     * @brief 获取所有告警
     * @return 告警列表
     */
    std::vector<Alert> getAllAlerts();
    
    /**
     * @brief 获取告警统计信息
     * @return 告警统计信息字符串
     */
    std::string getAlertStatistics();
    
    /**
     * @brief 根据指纹获取告警
     * @param fingerprint 告警指纹
     * @return 告警对象，如果不存在则返回nullptr
     */
    std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint);
    
    /**
     * @brief 根据ID获取告警
     * @param alertId 告警ID
     * @return 告警对象，如果不存在则返回nullptr
     */
    std::shared_ptr<Alert> getAlertById(const std::string& alertId);
    
    // 告警创建方法
    /**
     * @brief 主动创建告警
     * @param alertName 告警名称
     * @param alertType 告警类型
     * @param severity 严重程度
     * @param hostIp 主机IP地址
     * @param description 告警描述
     * @param summary 告警摘要
     * @param labels 标签（可选）
     * @param annotations 注释（可选）
     * @return 创建的告警对象，如果创建失败则返回nullptr
     */
    std::shared_ptr<Alert> createAlert(const std::string& alertName,
                                     const std::string& alertType,
                                     const std::string& severity,
                                     const std::string& hostIp,
                                     const std::string& description,
                                     const std::string& summary = "",
                                     const std::map<std::string, std::string>& labels = {},
                                     const std::map<std::string, std::string>& annotations = {});
    
    /**
     * @brief 创建组件状态告警（类似AlertRoutes中的/alert/component功能）
     * @param hostIp 主机IP地址
     * @param instanceId 组件实例ID
     * @param uuid 组件UUID
     * @param index 组件索引
     * @param status 组件状态
     * @return 创建的告警对象，如果创建失败则返回nullptr
     */
    std::shared_ptr<Alert> createAlertFromComponent(const std::string& hostIp,
                                                   const std::string& instanceId,
                                                   const std::string& uuid,
                                                   const std::string& index,
                                                   const std::string& status);
    
    // 告警数量查询方法
    /**
     * @brief 获取告警总数
     * @return 告警总数
     */
    size_t getAlertCount();
    
    /**
     * @brief 根据状态获取告警数量
     * @param status 告警状态
     * @return 该状态的告警数量
     */
    size_t getAlertCountByStatus(const std::string& status);
    
    /**
     * @brief 根据主机IP获取告警数量
     * @param hostIp 主机IP地址
     * @return 该主机的告警数量
     */
    size_t getAlertCountByHostIp(const std::string& hostIp);
    
    /**
     * @brief 根据告警规则ID获取告警数量
     * @param ruleId 告警规则ID
     * @return 该规则的告警数量
     */
    size_t getAlertCountByRuleId(const std::string& ruleId);
    
    /**
     * @brief 根据告警名称获取告警数量
     * @param alertName 告警名称
     * @return 该名称的告警数量
     */
    size_t getAlertCountByAlertName(const std::string& alertName);
    
    /**
     * @brief 根据告警类型获取告警数量
     * @param alertType 告警类型
     * @return 该类型的告警数量
     */
    size_t getAlertCountByAlertType(const std::string& alertType);
    
    /**
     * @brief 根据严重程度获取告警数量
     * @param severity 严重程度
     * @return 该严重程度的告警数量
     */
    size_t getAlertCountBySeverity(const std::string& severity);
    
    /**
     * @brief 根据时间范围获取告警数量
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @return 该时间范围内的告警数量
     */
    size_t getAlertCountByTimeRange(const std::string& startTime, const std::string& endTime);
    
    /**
     * @brief 获取活跃告警数量（firing状态）
     * @return 活跃告警数量
     */
    size_t getActiveAlertCount();
    
    /**
     * @brief 获取等待告警数量（pending状态）
     * @return 等待告警数量
     */
    size_t getPendingAlertCount();
    
    /**
     * @brief 获取已解决告警数量（resolved状态）
     * @return 已解决告警数量
     */
    size_t getResolvedAlertCount();

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
    
    /**
     * @brief 工作线程主循环
     */
    void workerLoop();
    
    /**
     * @brief 初始化告警引擎
     */
    void initialize();
    
    /**
     * @brief 执行一次完整的评估流程
     * @return 本次评估生成的告警数量
     */
    int performEvaluation();
    
    /**
     * @brief 评估所有告警规则
     * @return 生成的告警列表
     */
    std::vector<Alert> evaluateAllRules();
    
    /**
     * @brief 处理告警状态更新
     * @param currentAlerts 当前生成的告警列表
     * @return 处理的告警数量
     */
    int processAlertStatusUpdates(const std::vector<Alert>& currentAlerts);
    
    /**
     * @brief 更新告警到数据库
     * @param alerts 要更新的告警列表
     * @return 成功更新的告警数量
     */
    int updateAlertsToDatabase(const std::vector<Alert>& alerts);
    
    /**
     * @brief 获取当前数据库中firing状态的告警指纹集合
     * @return firing告警的指纹集合
     */
    std::unordered_set<std::string> getCurrentFiringFingerprints();
    
    /**
     * @brief 获取当前数据库中pending状态的告警指纹集合
     * @return pending告警的指纹集合
     */
    std::unordered_set<std::string> getCurrentPendingFingerprints();
    
    /**
     * @brief 将告警指纹集合转换为字符串（用于调试）
     * @param fingerprints 指纹集合
     * @return 指纹字符串
     */
    std::string fingerprintsToString(const std::unordered_set<std::string>& fingerprints);
    
    /**
     * @brief 生成唯一的告警ID
     * @return 生成的告警ID
     */
    std::string generateAlertId();
    
    /**
     * @brief 检查Pending状态的告警是否应该转为Firing
     * @param pendingAlert Pending状态的告警
     * @return 是否应该转为Firing
     */
    bool shouldTransitionToFiring(const Alert& pendingAlert);
    
    /**
     * @brief 解析持续时间字符串（支持s/m/h单位）
     * @param duration 持续时间字符串
     * @return 持续时间的秒数
     */
    int parseDuration(const std::string& duration);
    
    /**
     * @brief 解析ISO格式时间字符串
     * @param isoTime ISO格式时间字符串
     * @return 解析后的时间点
     */
    std::chrono::system_clock::time_point parseISOTime(const std::string& isoTime);
};

} // namespace alertv2
} // namespace yw
