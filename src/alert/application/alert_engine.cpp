// ============================================================================
// 文件功能描述：
// 告警引擎（AlertEngine）的实现文件，负责定期评估告警规则并生成告警事件。
// 主要功能包括：
// 1. 规则评估：定期执行所有启用的告警规则，评估条件是否满足
// 2. 告警生成：当规则条件满足时，创建或更新告警事件
// 3. 状态管理：管理告警事件的状态转换（Pending -> Firing -> Resolved）
// 4. 持续时间检查：检查告警是否持续满足条件达到指定的持续时间（for字段）
// 5. 回调通知：当告警状态变化时，调用推送回调函数通知外部系统
// 6. 统计信息：记录评估次数、生成的告警数量等统计信息
// 7. 线程管理：使用独立线程运行评估循环，支持优雅启动和停止
// ============================================================================

#include "alert_engine.h"
#include "alert_creation_factory.h"
#include "alert_rule_service.h"
#include "alert_query_service.h"
#include "../domain/alert_rule_evaluator.h"
#include "utils/json_config.h"
#include "utils/duration_utils.h"
#include "utils/time_utils.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace yw {
namespace alert {

// 告警引擎构造函数
// dbInterface: 数据库查询接口，不能为空
// alertRepo: 告警事件仓库，不能为空
// alertRuleService: 告警规则服务，不能为空
// 初始化告警工厂和查询服务，设置默认评估间隔为5秒
AlertEngine::AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                         std::shared_ptr<AlertEventRepository> alertRepo,
                         std::shared_ptr<AlertRuleService> alertRuleService)
    : dbInterface_(dbInterface), alertRepo_(alertRepo),
      alertRuleService_(alertRuleService),
      running_(false), shouldStop_(false), intervalSeconds_(5),
      totalEvaluations_(0), totalAlertsGenerated_(0),
      lastEvaluationTime_(std::chrono::system_clock::now()),
      startTime_(std::chrono::system_clock::now()) {
    
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
    if (!alertRepo_) {
        throw std::invalid_argument("AlertEventRepository不能为空");
    }
    if (!alertRuleService_) {
        throw std::invalid_argument("AlertRuleService不能为空");
    }
    
    // 初始化服务类
    alertFactory_ = std::make_shared<AlertCreationFactory>(alertRepo_, dbInterface_);
    alertQueryService_ = std::make_shared<AlertQueryService>(alertRepo_);
}

// 析构函数，自动停止告警引擎
AlertEngine::~AlertEngine() {
    stop();
}

// 启动告警引擎，初始化告警规则并启动工作线程
// intervalSeconds: 评估间隔（秒），默认5秒
void AlertEngine::start(int intervalSeconds) {
    if (running_) {
        spdlog::debug("告警引擎已经在运行中");
        return;
    }
    
    intervalSeconds_ = intervalSeconds;
    shouldStop_ = false;
    
    // 初始化告警规则
    initialize();
    
    // 启动工作线程
    running_ = true;
    workerThread_ = std::thread(&AlertEngine::workerLoop, this);
    
    // 启动节点存活检查（使用相同的间隔）
    if (alertFactory_) {
        alertFactory_->startAliveCheck(intervalSeconds);
    }
    
    spdlog::debug("告警引擎已启动，评估间隔: {} 秒", intervalSeconds_);
}

// 停止告警引擎，停止工作线程和节点存活检查
void AlertEngine::stop() {
    if (!running_) {
        return;
    }
    
    spdlog::debug("正在停止告警引擎...");
    shouldStop_ = true;
    
    // 停止节点存活检查
    if (alertFactory_) {
        alertFactory_->stopAliveCheck();
    }
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    running_ = false;
    spdlog::debug("告警引擎已停止");
}

// 告警引擎工作线程主循环
// 定期执行告警评估，检查间隔可中断
void AlertEngine::workerLoop() {
    spdlog::debug("告警引擎工作线程已启动");
    
    while (!shouldStop_) {
        try {
            // 执行评估
            performEvaluation();
            
            // 可中断的等待：分段 sleep，每次检查停止标志
            for (int i = 0; i < intervalSeconds_ * 10 && !shouldStop_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
        } catch (const std::exception& e) {
            spdlog::error("告警引擎评估过程中发生错误: {}", e.what());
            // 发生错误时等待一段时间再继续（可中断）
            for (int i = 0; i < intervalSeconds_ * 10 && !shouldStop_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    spdlog::debug("告警引擎工作线程已退出");
}

// 初始化告警引擎，重新加载告警规则
// 失败时抛出异常
void AlertEngine::initialize() {
    try {
        // 通过 AlertRuleService 重新加载规则
        alertRuleService_->reloadRules();
        spdlog::debug("告警引擎初始化完成");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("初始化告警引擎失败: " + std::string(e.what()));
    }
}

int AlertEngine::performEvaluation() {
    auto startTime = std::chrono::system_clock::now();
    
    try {
        spdlog::debug("=== 开始告警评估 ===");
        
        // 1. 评估所有告警规则，生成当前全量告警
        std::vector<AlertEvent> currentAlerts = evaluateAllRules();
        spdlog::debug("生成了 {} 个告警", currentAlerts.size());
        
        // 2. 处理告警状态更新
        int processedCount = processAlertStatusUpdates(currentAlerts);
        spdlog::debug("处理了 {} 个告警状态更新", processedCount);
        
        // 3. 更新告警到数据库
        int updatedCount = updateAlertsToDatabase(currentAlerts);
        spdlog::debug("更新了 {} 个告警到数据库", updatedCount);
        
        // 更新统计信息
        totalEvaluations_++;
        totalAlertsGenerated_ += currentAlerts.size();
        lastEvaluationTime_ = std::chrono::system_clock::now();
        
        auto endTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        spdlog::debug("告警评估完成，耗时: {} 毫秒", duration);
        
        return currentAlerts.size();
        
    } catch (const std::exception& e) {
        spdlog::error("告警评估失败: {}", e.what());
        throw;
    }
}

// 评估所有启用的告警规则，生成告警事件列表
// 返回: 所有规则生成的告警事件列表
std::vector<AlertEvent> AlertEngine::evaluateAllRules() {
    std::vector<AlertEvent> allAlerts;
                        
    // 从 AlertRuleService 获取所有启用的规则
    auto rules = alertRuleService_->getEnabledAlertRules();
    
    for (size_t i = 0; i < rules.size(); ++i) {
        const auto& rule = rules[i];
        
        try {
            // 使用AlertRuleEvaluator评估规则
            AlertRuleEvaluator evaluator(dbInterface_);
            auto alerts = evaluator.evaluateRule(rule);
            
            // 只有当生成了告警时才打印日志
            if (alerts.size() > 0) {
                spdlog::debug("规则 '{}' 生成了 {} 个告警", rule.getAlertName(), alerts.size());
            }
            
            // 将生成的告警添加到总列表
            allAlerts.insert(allAlerts.end(), alerts.begin(), alerts.end());
            
        } catch (const std::exception& e) {
            spdlog::error("评估规则 '{}' 时出错: {}", rule.getAlertName(), e.what());
        }
    }
    
    return allAlerts;
}

// 处理告警状态更新（Firing转Resolved，Pending删除）
// currentAlerts: 当前评估生成的告警列表
// 返回: 状态更新的告警数量
int AlertEngine::processAlertStatusUpdates(const std::vector<AlertEvent>& currentAlerts) {
    try {
        // 获取当前数据库中firing和pending状态的告警指纹（只获取硬件状态类型）
        auto currentFiringFingerprints = getCurrentFiringFingerprints();
        auto currentPendingFingerprints = getCurrentPendingFingerprints();
        spdlog::debug("数据库中当前firing告警数量（硬件状态）: {}", currentFiringFingerprints.size());
        spdlog::debug("数据库中当前pending告警数量（硬件状态）: {}", currentPendingFingerprints.size());
        
        // 获取当前生成的告警指纹
        std::unordered_set<std::string> currentAlertFingerprints;
        for (const auto& alert : currentAlerts) {
            currentAlertFingerprints.insert(alert.getFingerprint());
        }
        
        int processedCount = 0;
        
        // 1. 处理firing状态的告警：如果不再满足条件，转为resolved
        std::vector<std::string> firingAlertsToResolve;
        for (const auto& fingerprint : currentFiringFingerprints) {
            if (currentAlertFingerprints.find(fingerprint) == currentAlertFingerprints.end()) {
                firingAlertsToResolve.push_back(fingerprint);
            }
        }
        
        for (const auto& fingerprint : firingAlertsToResolve) {
            try {
                // 获取一个告警来检查节点是否有新数据
                auto alert = alertRepo_->getAlertByFingerprint(fingerprint);
                if (!alert) {
                    continue;
                }
                
                // 检查节点是否有新数据，如果没有新数据，保持告警状态不变
                if (!hasNodeRecentData(*alert)) {
                    // spdlog::debug("Firing告警节点无新数据，保持状态: {}", fingerprint);
                    continue;
                }
                
                // 节点有新数据但不满足条件，将所有相同fingerprint的firing告警都标记为resolved
                int resolvedCount = alertRepo_->resolveFiringAlertsByFingerprint(fingerprint);
                if (resolvedCount > 0) {
                    processedCount += resolvedCount;
                    spdlog::info("已解决 {} 个Firing告警: {}", resolvedCount, fingerprint);
                }
            } catch (const std::exception& e) {
                spdlog::error("解决firing告警 {} 时出错: {}", fingerprint, e.what());
            }
        }
        
        // 2. 处理pending状态的告警：如果不再满足条件，直接删除
        std::vector<std::string> pendingAlertsToDelete;
        for (const auto& fingerprint : currentPendingFingerprints) {
            if (currentAlertFingerprints.find(fingerprint) == currentAlertFingerprints.end()) {
                pendingAlertsToDelete.push_back(fingerprint);
            }
        }
        
        for (const auto& fingerprint : pendingAlertsToDelete) {
            try {
                auto alert = alertRepo_->getAlertByFingerprint(fingerprint);
                if (alert) {
                    // 检查节点是否有新数据，如果没有新数据，保持告警状态不变
                    if (!hasNodeRecentData(*alert)) {
                        //spdlog::debug("Pending告警节点无新数据，保持状态: {}", fingerprint);
                        continue;
                    }
                    // 节点有新数据但不满足条件，删除所有相同fingerprint的pending状态告警
                    int deletedCount = alertRepo_->deleteAlertsByFingerprintAndStatus(fingerprint, "pending");
                    if (deletedCount > 0) {
                        processedCount += deletedCount;
                        // spdlog::debug("已删除 {} 个Pending告警: {}", deletedCount, fingerprint);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("删除pending告警 {} 时出错: {}", fingerprint, e.what());
            }
        }
        
        return processedCount;
        
    } catch (const std::exception& e) {
        spdlog::error("处理告警状态更新时出错: {}", e.what());
        return 0;
    }
}

// 将告警列表更新到数据库
// alerts: 要更新的告警列表
// 返回: 成功更新的告警数量
int AlertEngine::updateAlertsToDatabase(const std::vector<AlertEvent>& alerts) {
    int successCount = 0;
    
    for (const auto& alert : alerts) {
        try {
            // 创建非const副本以调用updateInDatabase方法
            AlertEvent alertCopy = alert;
            
            // 检查数据库中是否已存在相同指纹的告警
            auto existingAlert = alertRepo_->getAlertByFingerprint(alertCopy.getFingerprint());
            
            if (existingAlert) {
                // 如果已存在告警，根据状态进行不同处理
                if (existingAlert->getStatus() == AlertStatus::Pending) {
                    if (alertCopy.getStatus() == AlertStatus::Pending) {
                        // 如果数据库中已有pending告警，且新告警也是pending，则跳过创建
                        // 只检查是否需要转为firing
                        if (shouldTransitionToFiring(*existingAlert)) {
                            existingAlert->transitionToFiring();
                            bool success = existingAlert->updateInDatabase(alertRepo_);
                            if (success) {
                                successCount++;
                                spdlog::info("Pending告警转为Firing: {}", alertCopy.getFingerprint());
                                
                                // 状态转换时调用推送回调
                                if (pushCallback_) {
                                    pushCallback_(*existingAlert);
                                }
                            }
                        } else {
                            // 保持pending状态，不需要更新数据库
                            spdlog::debug("Pending告警继续等待: {}", alertCopy.getFingerprint());
                        }
                        continue; // 跳过创建新告警
                    } else if (alertCopy.getStatus() == AlertStatus::Firing) {
                        // 如果数据库中已有pending告警，但新告警是firing，则转为firing
                        existingAlert->transitionToFiring();
                        bool success = existingAlert->updateInDatabase(alertRepo_);
                        if (success) {
                            successCount++;
                            spdlog::info("Pending告警转为Firing: {}", alertCopy.getFingerprint());
                            
                            // 状态转换时调用推送回调
                            if (pushCallback_) {
                                pushCallback_(*existingAlert);
                            }
                        }
                        continue; // 跳过创建新告警
                    }
                } else if (existingAlert->getStatus() == AlertStatus::Firing) {
                    // 如果数据库中已有firing告警，且新告警也是firing，则更新现有告警
                    existingAlert->setUpdatedNow();
                    bool success = existingAlert->updateInDatabase(alertRepo_);
                    if (success) {
                        successCount++;
                    }
                    continue; // 跳过创建新告警
                }
            }
            
            // 如果不存在现有告警，或者需要创建新告警，则正常处理
            // AlertEvent::updateInDatabase 方法会处理重复检查，所以这里可以安全调用
            bool success = alertCopy.updateInDatabase(alertRepo_);
            if (success) {
                successCount++;
                std::string alertValue = alertCopy.getLabel("value");
                spdlog::info("创建/更新告警: {} (状态: {}, 值: {})", 
                             alertCopy.getFingerprint(),
                             alertCopy.getStatus() == AlertStatus::Pending ? "Pending" : "Firing",
                             alertValue.empty() ? "N/A" : alertValue);
                
                // 只有新创建的firing告警才调用推送回调
                if (alertCopy.getStatus() == AlertStatus::Firing && pushCallback_) {
                    pushCallback_(alertCopy);
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("更新告警到数据库失败: {}", e.what());
        }
    }
    
    return successCount;
}

// 设置告警推送回调函数
// callback: 当创建或更新告警时调用的回调函数
void AlertEngine::setPushCallback(std::function<void(const AlertEvent&)> callback) {
    pushCallback_ = std::move(callback);
    // 同时设置到 AlertFactory
    if (alertFactory_) {
        alertFactory_->setPushCallback(pushCallback_);
    }
}

std::unordered_set<std::string> AlertEngine::getCurrentPendingFingerprints() {
    std::unordered_set<std::string> fingerprints;
    
    try {
        // 直接在 SQL 中过滤 alert_type，避免返回大量不需要的数据
        auto pendingAlerts = alertRepo_->getAlertsByStatusAndType("pending", "硬件状态");
        for (const auto& alert : pendingAlerts) {
            fingerprints.insert(alert.getFingerprint());
        }
    } catch (const std::exception& e) {
        spdlog::error("获取pending告警指纹时出错: {}", e.what());
    }
    
    return fingerprints;
}


// 私有辅助方法
bool AlertEngine::shouldTransitionToFiring(const AlertEvent& pendingAlert) {
    try {
        // 从告警的标签中获取告警名称
        auto labels = pendingAlert.getLabels();
        auto alertNameIt = labels.find("alert_name");
        if (alertNameIt == labels.end()) {
            spdlog::error("Pending告警缺少alert_name标签: {}", pendingAlert.getFingerprint());
            return false;
        }
        
        std::string alertName = alertNameIt->second;
        spdlog::debug("检查Pending告警是否应该转为Firing: {} (指纹: {})", alertName, pendingAlert.getFingerprint());
        
        // 查找对应的告警规则（从启用的规则中查找）
        AlertRule* rule = nullptr;
        auto rules = alertRuleService_->getEnabledAlertRules();
        for (auto& r : rules) {
            if (r.getAlertName() == alertName) {
                rule = &r;
                break;
            }
        }
        
        if (!rule) {
            spdlog::error("未找到告警规则: {}", alertName);
            return false;
        }
        
        // 解析for字段（持续时间）
        std::string forDuration = rule->getFor();
        
        if (forDuration.empty()) {
            // 如果没有设置for字段，默认立即转为firing
            spdlog::debug("for字段为空，立即转为Firing");
            return true;
        }
        
        // 解析持续时间（支持s/m/h单位）
        int durationSeconds = yw::utils::DurationUtils::parseToSeconds(forDuration);
        
        if (durationSeconds <= 0) {
            spdlog::debug("持续时间解析失败或为0，立即转为Firing");
            return true; // 解析失败，默认立即转为firing
        }
        
        // 计算告警创建时间到现在的持续时间
        std::string createdAt = pendingAlert.getCreatedAt();
        
        if (createdAt.empty()) {
            spdlog::error("告警创建时间为空");
            return false;
        }
        
        // 解析创建时间
        auto createdTime = yw::utils::TimeUtils::parseISOTime(createdAt);
        if (createdTime == std::chrono::system_clock::time_point{}) {
            spdlog::error("解析创建时间失败");
            return false;
        }
        
        // 计算持续时间
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - createdTime);
        
        spdlog::debug("告警已持续: {} 秒，需要: {} 秒", duration.count(), durationSeconds);
        
        // 如果持续时间大于等于for字段设置的时间，则转为firing
        bool shouldFire = duration.count() >= durationSeconds;
        spdlog::debug("是否应该转为Firing: {}", shouldFire ? "是" : "否");
        
        return shouldFire;
        
    } catch (const std::exception& e) {
        spdlog::error("检查Pending告警是否应该转为Firing时出错: {}", e.what());
        return false;
    }
}



std::unordered_set<std::string> AlertEngine::getCurrentFiringFingerprints() {
    std::unordered_set<std::string> fingerprints;
    
    try {
        // 直接在 SQL 中过滤 alert_type，避免返回大量不需要的数据
        auto firingAlerts = alertRepo_->getAlertsByStatusAndType("firing", "硬件状态");
        for (const auto& alert : firingAlerts) {
            fingerprints.insert(alert.getFingerprint());
        }
    } catch (const std::exception& e) {
        spdlog::error("获取firing告警指纹时出错: {}", e.what());
    }
    
    return fingerprints;
}

bool AlertEngine::hasNodeRecentData(const AlertEvent& alert, int seconds) {
    try {
        // 从告警的labels中获取必要信息
        std::string hostIp = alert.getLabel("host_ip");
        std::string stable = alert.getLabel("stable");
        
        if (hostIp.empty() || stable.empty()) {
            // 如果无法获取必要信息，假设有新数据（保守策略）
            return true;
        }
        
        // 获取表名
        std::string tableName;
        if (stable == "cpu") tableName = "resource_cpu";
        else if (stable == "memory") tableName = "resource_memory";
        else if (stable == "disk") tableName = "resource_disk";
        else if (stable == "network") tableName = "resource_network";
        else if (stable == "gpu") tableName = "resource_gpu";
        else if (stable == "alive") tableName = "resource_alive";
        else {
            // 未知的stable，假设有新数据
            return true;
        }
        
        // 构建查询，检查该节点在指定时间范围内是否有新数据
        // 使用参数化查询避免SQL注入
        std::ostringstream sql;
        std::vector<std::string> params;
        int paramIndex = 1;
        
        sql << "SELECT COUNT(*) as count FROM " << tableName;
        sql << " WHERE host_ip = $1::inet";
        sql << " AND time >= NOW() - INTERVAL '" << seconds << " seconds'";
        params.push_back(hostIp);
        paramIndex++;
        
        // 如果有标签列，添加标签条件（从告警的labels中获取）
        if (stable == "disk") {
            std::string device = alert.getLabel("device");
            std::string mountPoint = alert.getLabel("mount_point");
            if (!device.empty()) {
                sql << " AND device = $" << paramIndex;
                params.push_back(device);
                paramIndex++;
            }
            if (!mountPoint.empty()) {
                sql << " AND mount_point = $" << paramIndex;
                params.push_back(mountPoint);
                paramIndex++;
            }
        } else if (stable == "network") {
            std::string interface = alert.getLabel("interface");
            if (!interface.empty()) {
                sql << " AND interface = $" << paramIndex;
                params.push_back(interface);
                paramIndex++;
            }
        } else if (stable == "gpu") {
            std::string gpuIndex = alert.getLabel("gpu_index");
            if (!gpuIndex.empty()) {
                sql << " AND gpu_index = $" << paramIndex;
                params.push_back(gpuIndex);
                paramIndex++;
            }
        }
        
        QueryResult result = dbInterface_->executeQuery(sql.str(), params);
        
        // 检查是否有数据
        if (!result.rows.empty()) {
            std::string countStr = result.rows[0].getValue("count");
            int count = std::stoi(countStr);
            return count > 0;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("检查节点是否有新数据时出错: {}", e.what());
        // 出错时假设有新数据（保守策略）
        return true;
    }
}

} // namespace alert
} // namespace yw
