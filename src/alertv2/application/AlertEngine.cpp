#include "AlertEngine.h"
#include "../domain/AlertRuleEvaluator.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace yw {
namespace alertv2 {

AlertEngine::AlertEngine(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                         std::shared_ptr<AlertRuleRepository> alertRuleRepo,
                         std::shared_ptr<AlertRepository> alertRepo)
    : dbInterface_(dbInterface), alertRuleRepo_(alertRuleRepo), alertRepo_(alertRepo),
      running_(false), shouldStop_(false), intervalSeconds_(5),
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

int AlertEngine::evaluateOnce() {
    if (!running_) {
        std::cout << "告警引擎未运行，无法执行评估" << std::endl;
        return 0;
    }
    
    return performEvaluation();
}

void AlertEngine::reloadRules() {
    std::cout << "正在重新加载告警规则..." << std::endl;
    initialize();
    std::cout << "告警规则重新加载完成，共加载 " << rules_.size() << " 个规则" << std::endl;
}

std::string AlertEngine::getStatistics() const {
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();
    
    std::ostringstream oss;
    oss << "告警引擎统计信息:\n";
    oss << "  运行状态: " << (running_ ? "运行中" : "已停止") << "\n";
    oss << "  运行时间: " << uptime << " 秒\n";
    oss << "  告警规则数量: " << rules_.size() << "\n";
    oss << "  总评估次数: " << totalEvaluations_ << "\n";
    oss << "  总生成告警数: " << totalAlertsGenerated_ << "\n";
    oss << "  评估间隔: " << intervalSeconds_ << " 秒\n";
    
    if (totalEvaluations_ > 0) {
        oss << "  平均每次评估告警数: " << (totalAlertsGenerated_ / totalEvaluations_) << "\n";
    }
    
    return oss.str();
}

void AlertEngine::workerLoop() {
    std::cout << "告警引擎工作线程已启动" << std::endl;
    
    while (!shouldStop_) {
        try {
            // 执行评估
            performEvaluation();
            
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

std::vector<Alert> AlertEngine::evaluateAllRules() {
    std::vector<Alert> allAlerts;
    
    for (size_t i = 0; i < rules_.size(); ++i) {
        const auto& rule = rules_[i];
        
        try {
            // 使用AlertRuleEvaluator评估规则
            AlertRuleEvaluator evaluator(dbInterface_);
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
        // 获取当前数据库中firing和pending状态的告警指纹
        auto currentFiringFingerprints = getCurrentFiringFingerprints();
        auto currentPendingFingerprints = getCurrentPendingFingerprints();
        std::cout << "数据库中当前firing告警数量: " << currentFiringFingerprints.size() << std::endl;
        std::cout << "数据库中当前pending告警数量: " << currentPendingFingerprints.size() << std::endl;
        
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
                auto alert = alertRepo_->getAlertByFingerprint(fingerprint);
                if (alert) {
                    alert->transitionToResolved();
                    alert->updateInDatabase(alertRepo_);
                    processedCount++;
                    std::cout << "Firing告警已解决: " << fingerprint << std::endl;
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
                    // 直接删除pending状态的告警
                    bool deleted = alertRepo_->deleteAlert(alert->getId());
                    if (deleted) {
                        processedCount++;
                        std::cout << "Pending告警已删除: " << fingerprint << std::endl;
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
                    if (alertCopy.getStatus() == AlertStatus::Firing) {
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
            fingerprints.insert(alert.getFingerprint());
        }
    } catch (const std::exception& e) {
        std::cerr << "获取pending告警指纹时出错: " << e.what() << std::endl;
    }
    
    return fingerprints;
}

std::string AlertEngine::fingerprintsToString(const std::unordered_set<std::string>& fingerprints) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& fingerprint : fingerprints) {
        if (!first) {
            oss << ", ";
        }
        oss << fingerprint;
        first = false;
    }
    oss << "]";
    return oss.str();
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

bool AlertEngine::alertRuleExists(const std::string& ruleId) const {
    // 检查内存中是否存在
    for (const auto& rule : rules_) {
        if (rule.getId() == ruleId) {
            return true;
        }
    }
    
    // 检查数据库中是否存在
    try {
        return alertRuleRepo_->ruleExists(ruleId);
    } catch (const std::exception& e) {
        std::cerr << "检查告警规则是否存在时出错: " << e.what() << std::endl;
        return false;
    }
}

int AlertEngine::syncRulesFromDatabase() {
    try {
        std::cout << "正在同步数据库中的启用告警规则到内存..." << std::endl;
        
        // 从数据库获取所有启用的规则
        auto dbRules = alertRuleRepo_->getEnabledRules();
        
        // 清空内存中的规则
        rules_.clear();
        
        // 将数据库规则复制到内存
        rules_ = dbRules;
        
        std::cout << "成功同步 " << rules_.size() << " 个启用的告警规则到内存" << std::endl;
        return static_cast<int>(rules_.size());
        
    } catch (const std::exception& e) {
        std::cerr << "同步告警规则失败: " << e.what() << std::endl;
        return 0;
    }
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

std::vector<Alert> AlertEngine::getAlertsByRuleId(const std::string& ruleId) {
    try {
        return alertRepo_->getAlertsByRuleId(ruleId);
    } catch (const std::exception& e) {
        std::cerr << "根据规则ID获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAlertsByAlertName(const std::string& alertName) {
    try {
        return alertRepo_->getAlertsByAlertName(alertName);
    } catch (const std::exception& e) {
        std::cerr << "根据告警名称获取告警失败: " << e.what() << std::endl;
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

std::vector<Alert> AlertEngine::getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) {
    try {
        return alertRepo_->getAlertsByTimeRange(startTime, endTime);
    } catch (const std::exception& e) {
        std::cerr << "根据时间范围获取告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getRecentAlerts(int limit) {
    try {
        return alertRepo_->getRecentAlerts(limit);
    } catch (const std::exception& e) {
        std::cerr << "获取最近告警失败: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Alert> AlertEngine::getAllAlerts() {
    try {
        return alertRepo_->getAllAlerts();
    } catch (const std::exception& e) {
        std::cerr << "获取所有告警失败: " << e.what() << std::endl;
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

std::string AlertEngine::getAlertStatistics() {
    try {
        std::ostringstream oss;
        
        // 获取各种状态的告警数量
        auto firingAlerts = getAlertsByStatus("firing");
        auto resolvedAlerts = getAlertsByStatus("resolved");
        auto pendingAlerts = getAlertsByStatus("pending");
        
        oss << "=== 告警统计信息 ===\n";
        oss << "Firing告警数量: " << firingAlerts.size() << "\n";
        oss << "Resolved告警数量: " << resolvedAlerts.size() << "\n";
        oss << "Pending告警数量: " << pendingAlerts.size() << "\n";
        oss << "总告警数量: " << firingAlerts.size() + resolvedAlerts.size() + pendingAlerts.size() << "\n";
        
        // 按严重程度统计
        auto criticalAlerts = getAlertsBySeverity("严重");
        auto warningAlerts = getAlertsBySeverity("警告");
        auto infoAlerts = getAlertsBySeverity("信息");
        
        oss << "\n=== 按严重程度统计 ===\n";
        oss << "严重告警数量: " << criticalAlerts.size() << "\n";
        oss << "警告告警数量: " << warningAlerts.size() << "\n";
        oss << "信息告警数量: " << infoAlerts.size() << "\n";
        
        // 按告警类型统计
        auto hardwareAlerts = getAlertsByAlertType("硬件资源");
        auto availabilityAlerts = getAlertsByAlertType("availability");
        
        oss << "\n=== 按告警类型统计 ===\n";
        oss << "硬件资源告警数量: " << hardwareAlerts.size() << "\n";
        oss << "可用性告警数量: " << availabilityAlerts.size() << "\n";
        
        return oss.str();
        
    } catch (const std::exception& e) {
        std::cerr << "获取告警统计信息失败: " << e.what() << std::endl;
        return "获取告警统计信息失败: " + std::string(e.what());
    }
}

std::shared_ptr<Alert> AlertEngine::getAlertByFingerprint(const std::string& fingerprint) {
    try {
        return alertRepo_->getAlertByFingerprint(fingerprint);
    } catch (const std::exception& e) {
        std::cerr << "根据指纹获取告警失败: " << e.what() << std::endl;
        return nullptr;
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

size_t AlertEngine::getAlertCountByStatus(const std::string& status) {
    try {
        return alertRepo_->getAlertCountByStatus(status);
    } catch (const std::exception& e) {
        std::cerr << "根据状态获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getAlertCountByHostIp(const std::string& hostIp) {
    try {
        return alertRepo_->getAlertCountByHostIp(hostIp);
    } catch (const std::exception& e) {
        std::cerr << "根据主机IP获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getAlertCountByRuleId(const std::string& ruleId) {
    try {
        return alertRepo_->getAlertCountByRuleId(ruleId);
    } catch (const std::exception& e) {
        std::cerr << "根据规则ID获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getAlertCountByAlertName(const std::string& alertName) {
    try {
        return alertRepo_->getAlertCountByAlertName(alertName);
    } catch (const std::exception& e) {
        std::cerr << "根据告警名称获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getAlertCountByAlertType(const std::string& alertType) {
    try {
        return alertRepo_->getAlertCountByAlertType(alertType);
    } catch (const std::exception& e) {
        std::cerr << "根据告警类型获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getAlertCountBySeverity(const std::string& severity) {
    try {
        return alertRepo_->getAlertCountBySeverity(severity);
    } catch (const std::exception& e) {
        std::cerr << "根据严重程度获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getAlertCountByTimeRange(const std::string& startTime, const std::string& endTime) {
    try {
        return alertRepo_->getAlertCountByTimeRange(startTime, endTime);
    } catch (const std::exception& e) {
        std::cerr << "根据时间范围获取告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getActiveAlertCount() {
    try {
        return alertRepo_->getActiveAlertCount();
    } catch (const std::exception& e) {
        std::cerr << "获取活跃告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getPendingAlertCount() {
    try {
        return alertRepo_->getPendingAlertCount();
    } catch (const std::exception& e) {
        std::cerr << "获取等待告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

size_t AlertEngine::getResolvedAlertCount() {
    try {
        return alertRepo_->getResolvedAlertCount();
    } catch (const std::exception& e) {
        std::cerr << "获取已解决告警数量失败: " << e.what() << std::endl;
        return 0;
    }
}

// 告警创建方法实现
std::shared_ptr<Alert> AlertEngine::createAlert(const std::string& alertName,
                                               const std::string& alertType,
                                               const std::string& severity,
                                               const std::string& hostIp,
                                               const std::string& description,
                                               const std::string& summary,
                                               const std::map<std::string, std::string>& labels,
                                               const std::map<std::string, std::string>& annotations) {
    try {
        // 生成告警指纹（基于告警名称和主机IP）
        std::string fingerprint = hostIp + "_" + alertName;
        
        // 检查是否已存在相同的告警
        auto existingAlert = alertRepo_->getAlertByFingerprint(fingerprint);
        if (existingAlert) {
            // 如果已存在，更新现有告警
            existingAlert->setStatus(AlertStatus::Firing);
            existingAlert->setUpdatedNow();
            existingAlert->setStartsAt(existingAlert->getUpdatedAt());
            existingAlert->setEndsAt(""); // 清空结束时间
            
            // 更新标签和注释
            existingAlert->addLabel("description", description);
            if (!summary.empty()) {
                existingAlert->addLabel("summary", summary);
            }
            
            // 更新传入的标签和注释
            for (const auto& label : labels) {
                existingAlert->addLabel(label.first, label.second);
            }
            for (const auto& annotation : annotations) {
                existingAlert->addAnnotation(annotation.first, annotation.second);
            }
            
            // 保存到数据库
            bool success = existingAlert->updateInDatabase(alertRepo_);
            if (success) {
                std::cout << "更新现有告警: " << fingerprint << std::endl;
                
                // 手动创建的告警直接设为 Firing 状态，需要推送
                if (pushCallback_) {
                    pushCallback_(*existingAlert);
                }
                
                return existingAlert;
            } else {
                std::cerr << "更新现有告警失败: " << fingerprint << std::endl;
                return nullptr;
            }
        }
        
        // 创建新告警的标签和注释
        std::unordered_map<std::string, std::string> alertLabels;
        std::unordered_map<std::string, std::string> alertAnnotations;
        
        // 设置基本标签
        alertLabels["alert_name"] = alertName;
        alertLabels["alert_type"] = alertType;
        alertLabels["severity"] = severity;
        alertLabels["host_ip"] = hostIp;
        alertLabels["description"] = description;
        if (!summary.empty()) {
            alertLabels["summary"] = summary;
        }
        
        // 添加传入的标签和注释
        for (const auto& label : labels) {
            alertLabels[label.first] = label.second;
        }
        for (const auto& annotation : annotations) {
            alertAnnotations[annotation.first] = annotation.second;
        }
        
        // 创建新告警
        Alert newAlert(fingerprint, alertLabels, alertAnnotations);
        newAlert.setId(generateAlertId());
        newAlert.setStatus(AlertStatus::Firing);
        newAlert.setStartsAt(newAlert.getCreatedAt());
        newAlert.setEndsAt("");
        
        // 保存到数据库
        bool success = newAlert.updateInDatabase(alertRepo_);
        if (success) {
            std::cout << "成功创建新告警: " << fingerprint << " (" << alertName << ")" << std::endl;
            
            // 手动创建的新告警直接设为 Firing 状态，需要推送
            if (pushCallback_) {
                pushCallback_(newAlert);
            }
            
            return std::make_shared<Alert>(newAlert);
        } else {
            std::cerr << "创建告警失败: " << fingerprint << std::endl;
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "创建告警时发生错误: " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Alert> AlertEngine::createAlertFromComponent(const std::string& hostIp,
                                                           const std::string& instanceId,
                                                           const std::string& uuid,
                                                           const std::string& index,
                                                           const std::string& status) {
    try {
        // 生成指纹（与AlertRoutes中的逻辑保持一致）
        std::string fingerprint = hostIp + "_" + instanceId + "_" + uuid + "_" + index;
        
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
        alertLabels["alert_type"] = "availability";
        alertLabels["severity"] = "警告";
        alertLabels["host_ip"] = hostIp;
        alertLabels["instance_id"] = instanceId;
        alertLabels["uuid"] = uuid;
        alertLabels["index"] = index;
        alertLabels["component_status"] = status;
        
        // 设置描述
        std::string description = hostIp + " 节点上 " + instanceId + " 组件状态为 " + status;
        alertLabels["description"] = description;
        alertLabels["summary"] = "业务组件状态异常";
        
        // 设置注释
        alertAnnotations["component_type"] = "业务组件";
        alertAnnotations["monitoring_source"] = "external";
        
        // 创建新组件告警
        Alert newAlert(fingerprint, alertLabels, alertAnnotations);
        newAlert.setId(generateAlertId());
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
        std::cout << "开始解析ISO时间: " << isoTime << std::endl;
        
        // 解析ISO格式时间字符串 (例如: 2024-01-01T12:00:00.000Z)
        std::tm tm = {};
        std::istringstream ss(isoTime);
        
        // 处理不同的时间格式
        std::string timeStr = isoTime;
        bool isUTC = false;
        
        // 检查是否是PostgreSQL时间戳格式 (如: 2025-10-24 01:27:39.398+00)
        if (timeStr.find('+') != std::string::npos) {
            // 移除时区部分 (如: +00)
            size_t plusPos = timeStr.find('+');
            if (plusPos != std::string::npos) {
                timeStr = timeStr.substr(0, plusPos);
                isUTC = true;
                std::cout << "检测到PostgreSQL时间戳格式，移除时区部分" << std::endl;
            }
        } else if (timeStr.back() == 'Z') {
            // 处理标准ISO格式 (如: 2025-10-24T01:27:39.398Z)
            timeStr.pop_back();
            isUTC = true;
            std::cout << "检测到标准ISO时间格式" << std::endl;
        }
        
        std::cout << "处理后的时间字符串: " << timeStr << std::endl;
        
        // 解析时间（支持毫秒）
        // 重新创建istringstream使用处理后的时间字符串
        ss.clear();
        ss.str(timeStr);
        
        // 尝试解析不同的时间格式
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            // 如果T格式失败，尝试空格格式
            ss.clear();
            ss.str(timeStr);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) {
                std::cerr << "解析ISO时间失败，格式错误: " << isoTime << " (处理后: " << timeStr << ")" << std::endl;
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
        
        // 转换为time_point
        auto time_t = std::mktime(&tm);
        if (time_t == -1) {
            std::cerr << "解析ISO时间失败，mktime返回-1: " << isoTime << std::endl;
            return std::chrono::system_clock::time_point{};
        }
        
        std::cout << "mktime成功，time_t: " << time_t << std::endl;
        
        auto result = std::chrono::system_clock::from_time_t(time_t);
        
        // 添加毫秒
        result += std::chrono::milliseconds(milliseconds);
        
        // 如果是UTC时间，需要调整时区偏移
        if (isUTC) {
            // 获取本地时区偏移
            auto localTime = std::mktime(&tm);
            auto utcTime = std::mktime(std::gmtime(&time_t));
            auto offset = localTime - utcTime;
            result += std::chrono::seconds(offset);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "解析ISO时间失败: " << isoTime << " - " << e.what() << std::endl;
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
            fingerprints.insert(alert.getFingerprint());
        }
    } catch (const std::exception& e) {
        std::cerr << "获取firing告警指纹时出错: " << e.what() << std::endl;
    }
    
    return fingerprints;
}

} // namespace alertv2
} // namespace yw
