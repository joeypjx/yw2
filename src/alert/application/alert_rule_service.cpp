#include "alert_rule_service.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace yw {
namespace alert {

AlertRuleService::AlertRuleService(std::shared_ptr<AlertRuleRepository> alertRuleRepo)
    : alertRuleRepo_(alertRuleRepo) {
    if (!alertRuleRepo_) {
        throw std::invalid_argument("AlertRuleRepository不能为空");
    }
    // 初始化时加载所有启用的规则
    reloadRules();
}

bool AlertRuleService::addAlertRule(const AlertRule& rule) {
    try {
        // 1. 保存到数据库
        bool dbSuccess = alertRuleRepo_->saveRule(rule);
        if (!dbSuccess) {
            spdlog::error("保存告警规则到数据库失败: {}", rule.getId());
            return false;
        }
        
        // 2. 更新内存中的规则
        // 检查是否已存在
        auto it = std::find_if(rules_.begin(), rules_.end(),
                               [&rule](const AlertRule& r) {
                                   return r.getId() == rule.getId();
                               });
        
        if (it != rules_.end()) {
            // 如果已存在，更新它
            *it = rule;
            spdlog::debug("更新内存中的告警规则: {} ({})", rule.getId(), rule.getAlertName());
        } else {
            // 如果不存在，添加到内存
            rules_.push_back(rule);
            spdlog::debug("添加告警规则到内存: {} ({})", rule.getId(), rule.getAlertName());
        }
        
        // 3. 如果规则未启用，从内存中移除（只保留启用的规则，与reloadRules保持一致）
        if (!rule.isEnabled()) {
            rules_.erase(std::remove_if(rules_.begin(), rules_.end(),
                                       [&rule](const AlertRule& r) {
                                           return r.getId() == rule.getId();
                                       }),
                        rules_.end());
            spdlog::debug("告警规则未启用，已从内存中移除: {} ({})", rule.getId(), rule.getAlertName());
        }
        
        spdlog::debug("成功添加告警规则到数据库: {} ({})", rule.getId(), rule.getAlertName());
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("添加告警规则失败: {}", e.what());
        return false;
    }
}

bool AlertRuleService::updateAlertRule(const AlertRule& rule) {
    try {
        // 1. 更新数据库
        bool dbSuccess = alertRuleRepo_->saveRule(rule);
        if (!dbSuccess) {
            spdlog::error("更新告警规则到数据库失败: {}", rule.getId());
            return false;
        }
        
        // 2. 更新内存中的规则
        for (auto& existingRule : rules_) {
            if (existingRule.getId() == rule.getId()) {
                existingRule = rule;
                spdlog::debug("成功更新告警规则: {} ({})", rule.getId(), rule.getAlertName());
                return true;
            }
        }
        
        // 如果内存中没有找到，添加到内存
        rules_.push_back(rule);
        spdlog::debug("告警规则在内存中不存在，已添加到内存: {}", rule.getId());
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("更新告警规则失败: {}", e.what());
        return false;
    }
}

bool AlertRuleService::deleteAlertRule(const std::string& ruleId) {
    try {
        // 1. 从数据库删除
        bool dbSuccess = alertRuleRepo_->deleteRule(ruleId);
        if (!dbSuccess) {
            spdlog::error("从数据库删除告警规则失败: {}", ruleId);
            return false;
        }
        
        // 2. 从内存删除
        auto it = std::find_if(rules_.begin(), rules_.end(),
                               [&ruleId](const AlertRule& rule) {
                                   return rule.getId() == ruleId;
                               });
        
        if (it != rules_.end()) {
            rules_.erase(it);
            spdlog::debug("成功删除告警规则: {}", ruleId);
            return true;
        } else {
            spdlog::debug("告警规则在内存中不存在: {}", ruleId);
            return true; // 数据库已删除，即使内存中没有也算成功
        }
        
    } catch (const std::exception& e) {
        spdlog::error("删除告警规则失败: {}", e.what());
        return false;
    }
}

std::shared_ptr<AlertRule> AlertRuleService::getAlertRuleById(const std::string& ruleId) {
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
        spdlog::error("获取告警规则失败: {}", e.what());
        return nullptr;
    }
}

std::vector<AlertRule> AlertRuleService::getAllAlertRules() const {
    return rules_;
}

void AlertRuleService::reloadRules() {
    try {
        // 从数据库加载所有启用的告警规则
        rules_ = alertRuleRepo_->getEnabledRules();
        spdlog::debug("已加载 {} 个启用的告警规则到内存", rules_.size());
        
        if (rules_.empty()) {
            spdlog::warn("没有找到任何启用的告警规则");
        }
        
    } catch (const std::exception& e) {
        spdlog::error("重新加载告警规则失败: {}", e.what());
    }
}

} // namespace alert
} // namespace yw

