#include "AlertEngine.h"
#include "../domain/AlertRuleEvaluator.h"
#include "yw/JsonConfig.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace yw {
namespace alert {

AlertEngine::AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                         std::shared_ptr<AlertRuleRepository> alertRuleRepo,
                         std::shared_ptr<AlertRepository> alertRepo,
                         node::INodeModule* nodeModule)
    : dbInterface_(dbInterface), alertRuleRepo_(alertRuleRepo), alertRepo_(alertRepo),
      nodeModule_(nodeModule),
      running_(false), shouldStop_(false), intervalSeconds_(5),
      heartbeatTimeoutSeconds_(yw::utils::JsonConfig::Get<int>("alert.heartbeat_timeout_seconds", 5)),
      totalEvaluations_(0), totalAlertsGenerated_(0),
      lastEvaluationTime_(std::chrono::system_clock::now()),
      startTime_(std::chrono::system_clock::now()) {
    
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
    if (!alertRuleRepo_) {
        throw std::invalid_argument("AlertRuleRepository不能为空");
    }
    if (!alertRepo_) {
        throw std::invalid_argument("AlertRepository不能为空");
    }
}

AlertEngine::~AlertEngine() {
    stop();
}

void AlertEngine::start(int intervalSeconds) {
    if (running_) {
        std::cout << "告警引擎已经在运行中" << std::endl;
        return;
    }
    
    intervalSeconds_ = intervalSeconds;
    shouldStop_ = false;
    
    // 初始化告警规则
    initialize();
    
    // 启动工作线程
    running_ = true;
    workerThread_ = std::thread(&AlertEngine::workerLoop, this);
    
    std::cout << "告警引擎已启动，评估间隔: " << intervalSeconds_ << " 秒" << std::endl;
}

void AlertEngine::stop() {
    if (!running_) {
        return;
    }
    
    std::cout << "正在停止告警引擎..." << std::endl;
    shouldStop_ = true;
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    running_ = false;
    std::cout << "告警引擎已停止" << std::endl;
}

void AlertEngine::workerLoop() {
    std::cout << "告警引擎工作线程已启动" << std::endl;
    
    while (!shouldStop_) {
        try {
            // 执行评估
            performEvaluation();

            performEvaluationForAlive();
            
            // 等待下次评估
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds_));
            
        } catch (const std::exception& e) {
            std::cerr << "告警引擎评估过程中发生错误: " << e.what() << std::endl;
            // 发生错误时等待一段时间再继续
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds_));
        }
    }
    
    std::cout << "告警引擎工作线程已退出" << std::endl;
}

void AlertEngine::initialize() {
    try {
        // 从数据库加载所有启用的告警规则
        rules_ = alertRuleRepo_->getEnabledRules();
        std::cout << "已加载 " << rules_.size() << " 个启用的告警规则到内存" << std::endl;
        
        if (rules_.empty()) {
            std::cout << "警告: 没有找到任何启用的告警规则" << std::endl;
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error("初始化告警引擎失败: " + std::string(e.what()));
    }
}

int AlertEngine::performEvaluation() {
    auto startTime = std::chrono::system_clock::now();
    
    try {
        std::cout << "\n=== 开始告警评估 ===" << std::endl;
        
        // 1. 评估所有告警规则，生成当前全量告警
        std::vector<Alert> currentAlerts = evaluateAllRules();
        std::cout << "生成了 " << currentAlerts.size() << " 个告警" << std::endl;
        
        // 2. 处理告警状态更新
        int processedCount = processAlertStatusUpdates(currentAlerts);
        std::cout << "处理了 " << processedCount << " 个告警状态更新" << std::endl;
        
        // 3. 更新告警到数据库
        int updatedCount = updateAlertsToDatabase(currentAlerts);
        std::cout << "更新了 " << updatedCount << " 个告警到数据库" << std::endl;
        
        // 更新统计信息
        totalEvaluations_++;
        totalAlertsGenerated_ += currentAlerts.size();
        lastEvaluationTime_ = std::chrono::system_clock::now();
        
        auto endTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        std::cout << "告警评估完成，耗时: " << duration << " 毫秒" << std::endl;
        
        return currentAlerts.size();
        
    } catch (const std::exception& e) {
        std::cerr << "告警评估失败: " << e.what() << std::endl;
        throw;
    }
}

int AlertEngine::performEvaluationForAlive() {
    auto startTime = std::chrono::system_clock::now();
    
    try {
        std::cout << "\n=== 开始检查节点存活状态 ===" << std::endl;
        
        // 查询每个节点 IP 的最新时间，并在数据库层面直接计算时间差（秒）
        // 这样避免了时区转换的复杂性
        std::string sql = R"(
            SELECT 
                host_ip::text as host_ip, 
                MAX(time) as latest_time,
                EXTRACT(EPOCH FROM (NOW() - MAX(time)))::int as seconds_since_last_alive
            FROM resource_alive 
            GROUP BY host_ip 
            ORDER BY host_ip
        )";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        std::cout << "找到 " << result.size() << " 个节点" << std::endl;
        
        int alertCount = 0;
        
        // 遍历每个节点，检查最新时间
        for (const auto& row : result.rows) {
            std::string hostIp = row.getValue("host_ip");
            std::string latestTimeStr = row.getValue("latest_time");
            std::string secondsSinceLastAliveStr = row.getValue("seconds_since_last_alive");
            
            if (latestTimeStr.empty() || secondsSinceLastAliveStr.empty()) {
                std::cout << "节点 " << hostIp << ": 无记录" << std::endl;
                continue;
            }
            
            // 从数据库查询结果中获取时间差（秒）
            int secondsSinceLastAlive = 0;
            try {
                secondsSinceLastAlive = std::stoi(secondsSinceLastAliveStr);
            } catch (const std::exception& e) {
                std::cerr << "节点 " << hostIp << ": 解析时间差失败: " << secondsSinceLastAliveStr << std::endl;
                continue;
            }
            
            // 输出节点信息
            std::cout << "节点 " << hostIp 
                      << ": 最新心跳时间 " << latestTimeStr 
                      << ", 距离现在 " << secondsSinceLastAlive << " 秒" << std::endl;
            
            std::unordered_map<std::string, std::string> fingerprintTags;
            fingerprintTags["host_ip"] = hostIp;
            std::string fingerprint = Alert::generateFingerprint("节点心跳超时", fingerprintTags);

            // 如果超过配置的阈值未心跳，创建 firing 告警
            if (secondsSinceLastAlive > heartbeatTimeoutSeconds_) {
                try {
                    // 检查是否已存在相同 fingerprint 的 firing 告警
                    auto existingAlerts = alertRepo_->getAlertsByFingerprintAndStatus(fingerprint, "firing");
                    
                    if (existingAlerts.size() > 0) {
                        
                        // do not do anything
                    } else {
                        // 创建新告警的标签和注释
                        std::unordered_map<std::string, std::string> alertLabels;
                        std::unordered_map<std::string, std::string> alertAnnotations;
                        
                        // 设置基本标签
                        alertLabels["alert_name"] = "节点心跳超时";
                        alertLabels["alert_type"] = "availability";
                        alertLabels["severity"] = "严重";
                        alertLabels["host_ip"] = hostIp;
                        
                        // 设置描述
                        std::string description = "节点 " + hostIp + " 心跳超时，距离最新心跳已 " + 
                                                std::to_string(secondsSinceLastAlive) + " 秒";
                        alertLabels["description"] = description;
                        alertLabels["summary"] = "节点心跳超时";
                        
                        // 设置注释
                        alertAnnotations["description"] = description;
                        alertAnnotations["summary"] = "节点 " + hostIp + " 心跳超时";
                        alertAnnotations["monitoring_source"] = "alive_check";
                        alertAnnotations["last_alive_time"] = latestTimeStr;
                        alertAnnotations["seconds_since_last_alive"] = std::to_string(secondsSinceLastAlive);
                        
                        // 创建新告警
                        Alert newAlert(fingerprint, alertLabels, alertAnnotations);
                        newAlert.setStatus(AlertStatus::Firing);
                        newAlert.setStartsAt(newAlert.getCreatedAt());
                        newAlert.setEndsAt("");
                        
                        // 保存到数据库
                        bool success = newAlert.updateInDatabase(alertRepo_);
                        if (success) {
                            std::cout << "成功创建节点心跳超时告警: " << hostIp << " (指纹: " << fingerprint << ")" << std::endl;
                            alertCount++;
                            
                            // 节点心跳超时告警直接设为 Firing 状态，需要推送
                            if (pushCallback_) {
                                pushCallback_(newAlert);
                            }
                        } else {
                            std::cerr << "创建节点心跳超时告警失败: " << hostIp << std::endl;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "处理节点心跳超时告警时发生错误: " << hostIp << " - " << e.what() << std::endl;
                }
            } else {
                // 如果节点正常（<= 5秒），检查是否有已存在的告警需要解决
                try {
                    int resolvedCount = alertRepo_->resolveFiringAlertsByFingerprint(fingerprint);
                    if (resolvedCount > 0) {
                        std::cout << "已解决 " << resolvedCount << " 个节点心跳超时告警: " << hostIp << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "处理节点恢复时发生错误: " << hostIp << " - " << e.what() << std::endl;
                }
            }
        }
        
        auto endTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        std::cout << "节点存活状态检查完成，耗时: " << duration << " 毫秒，创建/更新了 " << alertCount << " 个告警" << std::endl;
        
        return result.size();
        
    } catch (const std::exception& e) {
        std::cerr << "检查节点存活状态失败: " << e.what() << std::endl;
        throw;
    }
}

std::vector<Alert> AlertEngine::evaluateAllRules() {
    std::vector<Alert> allAlerts;
    
    for (size_t i = 0; i < rules_.size(); ++i) {
        const auto& rule = rules_[i];
        
        try {
            // 使用AlertRuleEvaluator评估规则
            AlertRuleEvaluator evaluator(dbInterface_, nodeModule_);
            auto alerts = evaluator.evaluateRule(rule);
            
            std::cout << "规则 '" << rule.getAlertName() << "' 生成了 " << alerts.size() << " 个告警" << std::endl;
            
            // 将生成的告警添加到总列表
            allAlerts.insert(allAlerts.end(), alerts.begin(), alerts.end());
            
        } catch (const std::exception& e) {
            std::cerr << "评估规则 '" << rule.getAlertName() << "' 时出错: " << e.what() << std::endl;
        }
    }
    
    return allAlerts;
}

int AlertEngine::processAlertStatusUpdates(const std::vector<Alert>& currentAlerts) {
    try {
        // 获取当前数据库中firing和pending状态的告警指纹（只获取硬件资源类型）
        auto currentFiringFingerprints = getCurrentFiringFingerprints();
        auto currentPendingFingerprints = getCurrentPendingFingerprints();
        std::cout << "数据库中当前firing告警数量（硬件资源）: " << currentFiringFingerprints.size() << std::endl;
        std::cout << "数据库中当前pending告警数量（硬件资源）: " << currentPendingFingerprints.size() << std::endl;
        
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
                    std::cout << "Firing告警节点无新数据，保持状态: " << fingerprint << std::endl;
                    continue;
                }
                
                // 节点有新数据但不满足条件，将所有相同fingerprint的firing告警都标记为resolved
                int resolvedCount = alertRepo_->resolveFiringAlertsByFingerprint(fingerprint);
                if (resolvedCount > 0) {
                    processedCount += resolvedCount;
                    std::cout << "已解决 " << resolvedCount << " 个Firing告警: " << fingerprint << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "解决firing告警 " << fingerprint << " 时出错: " << e.what() << std::endl;
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
                        std::cout << "Pending告警节点无新数据，保持状态: " << fingerprint << std::endl;
                        continue;
                    }
                    // 节点有新数据但不满足条件，删除所有相同fingerprint的pending状态告警
                    int deletedCount = alertRepo_->deleteAlertsByFingerprintAndStatus(fingerprint, "pending");
                    if (deletedCount > 0) {
                        processedCount += deletedCount;
                        std::cout << "已删除 " << deletedCount << " 个Pending告警: " << fingerprint << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "删除pending告警 " << fingerprint << " 时出错: " << e.what() << std::endl;
            }
        }
        
        return processedCount;
        
    } catch (const std::exception& e) {
        std::cerr << "处理告警状态更新时出错: " << e.what() << std::endl;
        return 0;
    }
}

int AlertEngine::updateAlertsToDatabase(const std::vector<Alert>& alerts) {
    int successCount = 0;
    
    for (const auto& alert : alerts) {
        try {
            // 创建非const副本以调用updateInDatabase方法
            Alert alertCopy = alert;
            
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
                                std::cout << "Pending告警转为Firing: " << alertCopy.getFingerprint() << std::endl;
                                
                                // 状态转换时调用推送回调
                                if (pushCallback_) {
                                    pushCallback_(*existingAlert);
                                }
                            }
                        } else {
                            // 保持pending状态，不需要更新数据库
                            std::cout << "Pending告警继续等待: " << alertCopy.getFingerprint() << std::endl;
                        }
                        continue; // 跳过创建新告警
                    } else if (alertCopy.getStatus() == AlertStatus::Firing) {
                        // 如果数据库中已有pending告警，但新告警是firing，则转为firing
                        existingAlert->transitionToFiring();
                        bool success = existingAlert->updateInDatabase(alertRepo_);
                        if (success) {
                            successCount++;
                            std::cout << "Pending告警转为Firing: " << alertCopy.getFingerprint() << std::endl;
                            
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
                        std::cout << "更新现有Firing告警: " << alertCopy.getFingerprint() << std::endl;
                    }
                    continue; // 跳过创建新告警
                }
            }
            
            // 如果不存在现有告警，或者需要创建新告警，则正常处理
            // Alert::updateInDatabase 方法会处理重复检查，所以这里可以安全调用
            bool success = alertCopy.updateInDatabase(alertRepo_);
            if (success) {
                successCount++;
                std::cout << "创建/更新告警: " << alertCopy.getFingerprint() << " (状态: " 
                         << (alertCopy.getStatus() == AlertStatus::Pending ? "Pending" : "Firing") << ")" << std::endl;
                
                // 只有新创建的firing告警才调用推送回调
                if (alertCopy.getStatus() == AlertStatus::Firing && pushCallback_) {
                    pushCallback_(alertCopy);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "更新告警到数据库失败: " << e.what() << std::endl;
        }
    }
    
    return successCount;
}

void AlertEngine::setPushCallback(std::function<void(const Alert&)> callback) {
    pushCallback_ = std::move(callback);
}

std::unordered_set<std::string> AlertEngine::getCurrentPendingFingerprints() {
    std::unordered_set<std::string> fingerprints;
    
    try {
        auto pendingAlerts = alertRepo_->getAlertsByStatus("pending");
        for (const auto& alert : pendingAlerts) {
            // 只获取 alert_type 为 "硬件状态" 的告警
            std::string alertType = alert.getLabel("alert_type");
            if (alertType == "硬件状态") {
                fingerprints.insert(alert.getFingerprint());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "获取pending告警指纹时出错: " << e.what() << std::endl;
    }
    
    return fingerprints;
}

// 告警规则管理方法实现
bool AlertEngine::addAlertRule(const AlertRule& rule) {
    try {
        // 1. 保存到数据库
        bool dbSuccess = alertRuleRepo_->saveRule(rule);
        if (!dbSuccess) {
            std::cerr << "保存告警规则到数据库失败: " << rule.getId() << std::endl;
            return false;
        }
        
        // 2. 添加到内存
        rules_.push_back(rule);
        
        std::cout << "成功添加告警规则: " << rule.getId() << " (" << rule.getAlertName() << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "添加告警规则失败: " << e.what() << std::endl;
        return false;
    }
}

bool AlertEngine::updateAlertRule(const AlertRule& rule) {
    try {
        // 1. 更新数据库
        bool dbSuccess = alertRuleRepo_->saveRule(rule);
        if (!dbSuccess) {
            std::cerr << "更新告警规则到数据库失败: " << rule.getId() << std::endl;
            return false;
        }
        
        // 2. 更新内存中的规则
        for (auto& existingRule : rules_) {
            if (existingRule.getId() == rule.getId()) {
                existingRule = rule;
                std::cout << "成功更新告警规则: " << rule.getId() << " (" << rule.getAlertName() << ")" << std::endl;
                return true;
            }
        }
        
        // 如果内存中没有找到，添加到内存
        rules_.push_back(rule);
        std::cout << "告警规则在内存中不存在，已添加到内存: " << rule.getId() << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "更新告警规则失败: " << e.what() << std::endl;
        return false;
    }
}

bool AlertEngine::deleteAlertRule(const std::string& ruleId) {
    try {
        // 1. 从数据库删除
        bool dbSuccess = alertRuleRepo_->deleteRule(ruleId);
        if (!dbSuccess) {
            std::cerr << "从数据库删除告警规则失败: " << ruleId << std::endl;
            return false;
        }
        
        // 2. 从内存删除
        auto it = std::find_if(rules_.begin(), rules_.end(),
                               [&ruleId](const AlertRule& rule) {
                                   return rule.getId() == ruleId;
                               });
        
        if (it != rules_.end()) {
            rules_.erase(it);
            std::cout << "成功删除告警规则: " << ruleId << std::endl;
            return true;
        } else {
            std::cout << "告警规则在内存中不存在: " << ruleId << std::endl;
            return true; // 数据库已删除，即使内存中没有也算成功
        }
        
    } catch (const std::exception& e) {
        std::cerr << "删除告警规则失败: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<AlertRule> AlertEngine::getAlertRuleById(const std::string& ruleId) {
    try {
        // 先从内存查找
        for (const auto& rule : rules_) {
            if (rule.getId() == ruleId) {
                return std::make_shared<AlertRule>(rule);
            }
        }
        
        // 如果内存中没有，从数据库查找
        auto rule = alertRuleRepo_->getRuleById(ruleId);
        if (rule) {
            // 添加到内存中
            rules_.push_back(*rule);
            return rule;
        }
        
        return nullptr;
        
    } catch (const std::exception& e) {
        std::cerr << "获取告警规则失败: " << e.what() << std::endl;
        return nullptr;
    }
}

std::vector<AlertRule> AlertEngine::getAllAlertRules() const {
    return rules_;
}

// 告警查询方法实现
std::vector<Alert> AlertEngine::getAlertsByStatus(const std::string& status) {
    try {
        return alertRepo_->getAlertsByStatus(status);
    } catch (const std::exception& e) {
        std::cerr << "根据状态获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByHostIp(const std::string& hostIp) {
    try {
        return alertRepo_->getAlertsByHostIp(hostIp);
    } catch (const std::exception& e) {
        std::cerr << "根据主机IP获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByBoxId(int boxId) {
    try {
        return alertRepo_->getAlertsByBoxId(boxId);
    } catch (const std::exception& e) {
        std::cerr << "根据机箱号获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsBySlotId(int slotId) {
    try {
        return alertRepo_->getAlertsBySlotId(slotId);
    } catch (const std::exception& e) {
        std::cerr << "根据板卡号获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) {
    try {
        return alertRepo_->getAlertsByTimeRange(startTime, endTime);
    } catch (const std::exception& e) {
        std::cerr << "根据时间范围获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByAlertType(const std::string& alertType) {
    try {
        return alertRepo_->getAlertsByAlertType(alertType);
    } catch (const std::exception& e) {
        std::cerr << "根据告警类型获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsBySeverity(const std::string& severity) {
    try {
        return alertRepo_->getAlertsBySeverity(severity);
    } catch (const std::exception& e) {
        std::cerr << "根据严重程度获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByDescription(const std::string& description) {
    try {
        return alertRepo_->getAlertsByDescription(description);
    } catch (const std::exception& e) {
        std::cerr << "根据描述获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsExceptPending() {
    try {
        return alertRepo_->getAlertsExceptPending();
    } catch (const std::exception& e) {
        std::cerr << "获取除Pending外的告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByFilters(const AlertFilters& filters) {
    try {
        return alertRepo_->getAlertsByFilters(filters);
    } catch (const std::exception& e) {
        std::cerr << "根据过滤条件获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::shared_ptr<Alert> AlertEngine::getAlertById(const std::string& alertId) {
    try {
        return alertRepo_->getAlertById(alertId);
    } catch (const std::exception& e) {
        std::cerr << "根据ID获取告警失败: " << e.what() << std::endl;
        return nullptr;
    }
}

// 告警数量查询方法实现
size_t AlertEngine::getAlertCount() {
    try {
        return alertRepo_->getAlertCount();
    } catch (const std::exception& e) {
        std::cerr << "获取告警总数失败: " << e.what() << std::endl;
        return 0;
    }
}

// 告警创建方法实现
std::shared_ptr<Alert> AlertEngine::createAlertFromComponent(const std::string& hostIp,
                                                           const std::string& instanceId,
                                                           const std::string& uuid,
                                                           int index,
                                                           const std::string& status,
                                                           const std::string& stack_name,
                                                           const std::string& component_name) {
    try {
        // 生成指纹（与AlertRoutes中的逻辑保持一致）
        std::string fingerprint = "业务组件状态异常|host_ip=" + hostIp + "|instance_id=" + instanceId + "|uuid=" + uuid + "|index=" + std::to_string(index);
        
        // 检查是否已存在相同的告警
        auto existingAlert = alertRepo_->getAlertByFingerprint(fingerprint);
        if (existingAlert) {
            // 如果已存在，更新现有告警
            existingAlert->setStatus(AlertStatus::Firing);
            existingAlert->setUpdatedNow();
            existingAlert->setStartsAt(existingAlert->getUpdatedAt());
            existingAlert->setEndsAt(""); // 清空结束时间
            
            // 更新描述
            std::string description = hostIp + " 节点上 " + instanceId + " 组件状态为 " + status;
            existingAlert->addLabel("description", description);
            
            // 保存到数据库
            bool success = existingAlert->updateInDatabase(alertRepo_);
            if (success) {
                std::cout << "更新现有组件告警: " << fingerprint << std::endl;
                
                // 组件告警直接设为 Firing 状态，需要推送
                if (pushCallback_) {
                    pushCallback_(*existingAlert);
                }
                
                return existingAlert;
            } else {
                std::cerr << "更新现有组件告警失败: " << fingerprint << std::endl;
                return nullptr;
            }
        }
        
        // 创建新组件告警的标签和注释
        std::unordered_map<std::string, std::string> alertLabels;
        std::unordered_map<std::string, std::string> alertAnnotations;

        // 设置基本标签
        alertLabels["alert_name"] = "业务组件状态异常";
        alertLabels["alert_type"] = "业务链路";
        alertLabels["severity"] = "严重";
        alertLabels["host_ip"] = hostIp;
        alertLabels["instance_id"] = instanceId;
        alertLabels["uuid"] = uuid;
        alertLabels["index"] = std::to_string(index);
        alertLabels["value"] = status;
        alertLabels["stack_name"] = stack_name;
        alertLabels["component_name"] = component_name;
        
        // 设置描述
        std::string description = hostIp + " 节点上 " + instanceId + " 组件状态为 " + status;
        alertAnnotations["summary"] = "业务组件状态异常";
        alertAnnotations["description"] = description;
        
        // 创建新组件告警
        Alert newAlert(fingerprint, alertLabels, alertAnnotations);
        newAlert.setStatus(AlertStatus::Firing);
        newAlert.setStartsAt(newAlert.getCreatedAt());
        newAlert.setEndsAt("");
        
        // 保存到数据库
        bool success = newAlert.updateInDatabase(alertRepo_);
        if (success) {
            std::cout << "成功创建组件告警: " << fingerprint << " (组件: " << instanceId << ")" << std::endl;
            
            // 组件告警直接设为 Firing 状态，需要推送
            if (pushCallback_) {
                pushCallback_(newAlert);
            }
            
            return std::make_shared<Alert>(newAlert);
        } else {
            std::cerr << "创建组件告警失败: " << fingerprint << std::endl;
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "创建组件告警时发生错误: " << e.what() << std::endl;
        return nullptr;
    }
}

// 私有辅助方法
bool AlertEngine::shouldTransitionToFiring(const Alert& pendingAlert) {
    try {
        // 从告警的标签中获取告警名称
        auto labels = pendingAlert.getLabels();
        auto alertNameIt = labels.find("alert_name");
        if (alertNameIt == labels.end()) {
            std::cerr << "Pending告警缺少alert_name标签: " << pendingAlert.getFingerprint() << std::endl;
            return false;
        }
        
        std::string alertName = alertNameIt->second;
        std::cout << "检查Pending告警是否应该转为Firing: " << alertName << " (指纹: " << pendingAlert.getFingerprint() << ")" << std::endl;
        
        // 查找对应的告警规则
        AlertRule* rule = nullptr;
        for (auto& r : rules_) {
            if (r.getAlertName() == alertName) {
                rule = &r;
                break;
            }
        }
        
        if (!rule) {
            std::cerr << "未找到告警规则: " << alertName << std::endl;
            return false;
        }
        
        // 解析for字段（持续时间）
        std::string forDuration = rule->getFor();
        std::cout << "告警规则的for字段: '" << forDuration << "'" << std::endl;
        
        if (forDuration.empty()) {
            // 如果没有设置for字段，默认立即转为firing
            std::cout << "for字段为空，立即转为Firing" << std::endl;
            return true;
        }
        
        // 解析持续时间（支持s/m/h单位）
        int durationSeconds = parseDuration(forDuration);
        std::cout << "解析的持续时间: " << durationSeconds << " 秒" << std::endl;
        
        if (durationSeconds <= 0) {
            std::cout << "持续时间解析失败或为0，立即转为Firing" << std::endl;
            return true; // 解析失败，默认立即转为firing
        }
        
        // 计算告警创建时间到现在的持续时间
        std::string createdAt = pendingAlert.getCreatedAt();
        std::cout << "告警创建时间: " << createdAt << std::endl;
        
        if (createdAt.empty()) {
            std::cerr << "告警创建时间为空" << std::endl;
            return false;
        }
        
        // 解析创建时间
        auto createdTime = parseISOTime(createdAt);
        if (createdTime == std::chrono::system_clock::time_point{}) {
            std::cerr << "解析创建时间失败" << std::endl;
            return false;
        }
        
        // 计算持续时间
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - createdTime);
        
        std::cout << "告警已持续: " << duration.count() << " 秒，需要: " << durationSeconds << " 秒" << std::endl;
        
        // 如果持续时间大于等于for字段设置的时间，则转为firing
        bool shouldFire = duration.count() >= durationSeconds;
        std::cout << "是否应该转为Firing: " << (shouldFire ? "是" : "否") << std::endl;
        
        return shouldFire;
        
    } catch (const std::exception& e) {
        std::cerr << "检查Pending告警是否应该转为Firing时出错: " << e.what() << std::endl;
        return false;
    }
}

int AlertEngine::parseDuration(const std::string& duration) {
    try {
        if (duration.empty()) {
            return 0;
        }
        
        // 提取数字部分
        std::string numberStr;
        char unit = 's'; // 默认单位
        
        for (char c : duration) {
            if (std::isdigit(c)) {
                numberStr += c;
            } else if (c == 's' || c == 'm' || c == 'h') {
                unit = c;
                break;
            }
        }
        
        if (numberStr.empty()) {
            return 0;
        }
        
        int number = std::stoi(numberStr);
        
        // 转换为秒
        switch (unit) {
            case 's':
                return number;
            case 'm':
                return number * 60;
            case 'h':
                return number * 3600;
            default:
                return number;
        }
    } catch (const std::exception& e) {
        std::cerr << "解析持续时间失败: " << duration << " - " << e.what() << std::endl;
        return 0;
    }
}

std::chrono::system_clock::time_point AlertEngine::parseISOTime(const std::string& isoTime) {
    try {
        std::cout << "开始解析时间: " << isoTime << std::endl;
        
        // 解析时间字符串 (例如: 2024-01-01T12:00:00.000)
        std::tm tm = {};
        std::istringstream ss(isoTime);
        
        // 处理时间字符串，移除Z后缀（如果存在）
        std::string timeStr = isoTime;
        if (timeStr.back() == 'Z') {
            timeStr.pop_back();
        }
        
        std::cout << "处理后的时间字符串: " << timeStr << std::endl;
        
        // 解析时间（支持毫秒）
        ss.str(timeStr);
        
        // 尝试解析不同的时间格式
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            // 如果T格式失败，尝试空格格式
            ss.clear();
            ss.str(timeStr);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) {
                std::cerr << "解析时间失败，格式错误: " << isoTime << " (处理后: " << timeStr << ")" << std::endl;
                return std::chrono::system_clock::time_point{};
            }
        }
        
        // 处理毫秒部分（如果存在）
        int milliseconds = 0;
        if (ss.peek() == '.') {
            ss.ignore(); // 跳过 '.'
            ss >> milliseconds;
            if (ss.fail()) {
                milliseconds = 0;
            }
            std::cout << "解析到毫秒: " << milliseconds << std::endl;
        }
        
        // 转换为time_point（使用本地时间）
        auto time_t = std::mktime(&tm);
        if (time_t == -1) {
            std::cerr << "解析时间失败，mktime返回-1: " << isoTime << std::endl;
            return std::chrono::system_clock::time_point{};
        }
        
        std::cout << "mktime成功，time_t: " << time_t << std::endl;
        
        auto result = std::chrono::system_clock::from_time_t(time_t);
        
        // 添加毫秒
        result += std::chrono::milliseconds(milliseconds);
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "解析时间失败: " << isoTime << " - " << e.what() << std::endl;
        return std::chrono::system_clock::time_point{};
    }
}

std::string AlertEngine::generateAlertId() {
    // 生成基于时间戳的唯一ID
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "alert_" << time_t << "_" << ms.count();
    return ss.str();
}

std::unordered_set<std::string> AlertEngine::getCurrentFiringFingerprints() {
    std::unordered_set<std::string> fingerprints;
    
    try {
        auto firingAlerts = alertRepo_->getAlertsByStatus("firing");
        for (const auto& alert : firingAlerts) {
            // 只获取 alert_type 为 "硬件状态" 的告警
            std::string alertType = alert.getLabel("alert_type");
            if (alertType == "硬件状态") {
                fingerprints.insert(alert.getFingerprint());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "获取firing告警指纹时出错: " << e.what() << std::endl;
    }
    
    return fingerprints;
}

bool AlertEngine::hasNodeRecentData(const Alert& alert, int seconds) {
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
        std::cerr << "检查节点是否有新数据时出错: " << e.what() << std::endl;
        // 出错时假设有新数据（保守策略）
        return true;
    }
}

} // namespace alert
} // namespace yw
