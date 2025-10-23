#include "AlertRuleTester.h"
#include "../domain/AlertRule.h"
#include "../domain/Alert.h"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace yw {
namespace alertv2 {

AlertRuleTester::AlertRuleTester(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                                 std::shared_ptr<AlertRepository> alertRepo)
    : dbInterface_(dbInterface), alertRepo_(alertRepo), shouldStop_(false) {
    repository_ = std::make_shared<DatabaseAlertRuleRepository>(dbInterface_);
}

void AlertRuleTester::runTest(int intervalSeconds, int maxRounds) {
    std::cout << "=== AlertRuleTester 开始运行 ===" << std::endl;
    std::cout << "测试间隔: " << intervalSeconds << " 秒" << std::endl;
    std::cout << "最大轮数: " << (maxRounds == 0 ? "无限" : std::to_string(maxRounds)) << std::endl;
    std::cout << "按 Ctrl+C 停止测试" << std::endl;
    std::cout << std::endl;
    
    int round = 0;
    while (!shouldStop_ && (maxRounds == 0 || round < maxRounds)) {
        round++;
        
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        std::cout << "=== 第 " << round << " 轮测试 ===" << std::endl;
        
        try {
            int alertCount = runSingleTest();
            std::cout << "本轮测试完成，生成 " << alertCount << " 个告警" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "测试出错: " << e.what() << std::endl;
        }
        
        std::cout << std::endl;
        
        // 如果不是最后一轮，等待指定时间
        if (!shouldStop_ && (maxRounds == 0 || round < maxRounds)) {
            std::cout << "等待 " << intervalSeconds << " 秒..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
    }
    
    std::cout << "=== AlertRuleTester 测试结束 ===" << std::endl;
}

int AlertRuleTester::runSingleTest() {
    try {
        // 1. 从数据库获取所有告警规则
        std::cout << "正在从数据库获取告警规则..." << std::endl;
        auto rules = repository_->getAllRules();
        std::cout << "获取到 " << rules.size() << " 个告警规则" << std::endl;
        
        if (rules.empty()) {
            std::cout << "没有找到告警规则，跳过测试" << std::endl;
            return 0;
        }
        
        // 2. 打印告警规则信息
        printRules(rules);
        
        // 3. 循环评估每个告警规则
        int totalAlerts = 0;
        for (size_t i = 0; i < rules.size(); ++i) {
            const auto& rule = rules[i];
            std::cout << "\n--- 评估告警规则 " << (i + 1) << ": " << rule.getAlertName() << " ---" << std::endl;
            
            try {
                // 运行evaluate方法
                auto alerts = rule.evaluate(dbInterface_);
                std::cout << "规则 '" << rule.getAlertName() << "' 生成了 " << alerts.size() << " 个告警" << std::endl;
                
                if (!alerts.empty()) {
                    printAlerts(alerts);
                    
                    // 如果提供了告警存储Repository，则将告警存储到数据库
                    if (alertRepo_) {
                        std::cout << "正在将告警存储到数据库..." << std::endl;
                        int savedCount = 0;
                        for (const auto& alert : alerts) {
                            try {
                                // 为告警添加告警规则ID到labels中
                                auto labels = alert.getLabels();
                                labels["alert_rule_id"] = rule.getId();
                                
                                // 创建新的告警对象（包含告警规则ID）
                                Alert alertWithRuleId(alert.getFingerprint(), labels, alert.getAnnotations());
                                alertWithRuleId.setId(alert.getId());
                                alertWithRuleId.setStatus(alert.getStatus());
                                alertWithRuleId.setCreatedAt(alert.getCreatedAt());
                                alertWithRuleId.setStartsAt(alert.getStartsAt());
                                alertWithRuleId.setUpdatedAt(alert.getUpdatedAt());
                                alertWithRuleId.setEndsAt(alert.getEndsAt());
                                
                                if (alertRepo_->saveAlert(alertWithRuleId)) {
                                    savedCount++;
                                }
                            } catch (const std::exception& e) {
                                std::cout << "保存告警失败: " << e.what() << std::endl;
                            }
                        }
                        std::cout << "成功存储 " << savedCount << " 个告警到数据库" << std::endl;
                    }
                }
                
                totalAlerts += alerts.size();
                
            } catch (const std::exception& e) {
                std::cout << "评估规则 '" << rule.getAlertName() << "' 时出错: " << e.what() << std::endl;
            }
        }
        
        return totalAlerts;
        
    } catch (const std::exception& e) {
        std::cout << "测试过程中发生错误: " << e.what() << std::endl;
        throw;
    }
}

void AlertRuleTester::stopTest() {
    shouldStop_ = true;
}

void AlertRuleTester::printRules(const std::vector<AlertRule>& rules) {
    std::cout << "\n=== 告警规则列表 ===" << std::endl;
    for (size_t i = 0; i < rules.size(); ++i) {
        const auto& rule = rules[i];
        std::cout << "规则 " << (i + 1) << ":" << std::endl;
        std::cout << "  ID: " << rule.getId() << std::endl;
        std::cout << "  名称: " << rule.getAlertName() << std::endl;
        std::cout << "  类型: " << rule.getAlertType() << std::endl;
        std::cout << "  严重程度: " << rule.getSeverity() << std::endl;
        std::cout << "  持续时间: " << rule.getFor() << std::endl;
        std::cout << "  摘要: " << rule.getSummary() << std::endl;
        std::cout << "  表达式: " << rule.toJson()["expression"].dump(2) << std::endl;
        std::cout << "  创建时间: " << rule.getCreatedAt() << std::endl;
        std::cout << "  更新时间: " << rule.getUpdatedAt() << std::endl;
        std::cout << std::endl;
    }
}

void AlertRuleTester::printAlerts(const std::vector<Alert>& alerts) {
    std::cout << "\n=== 生成的告警 ===" << std::endl;
    for (size_t i = 0; i < alerts.size(); ++i) {
        const auto& alert = alerts[i];
        std::cout << "告警 " << (i + 1) << ":" << std::endl;
        std::cout << "  ID: " << alert.getId() << std::endl;
        std::cout << "  指纹: " << alert.getFingerprint() << std::endl;
        std::cout << "  状态: " << (alert.getStatus() == AlertStatus::Firing ? "firing" : 
                                    alert.getStatus() == AlertStatus::Pending ? "pending" : "resolved") << std::endl;
        std::cout << "  标签: " << nlohmann::json(alert.getLabels()).dump(2) << std::endl;
        std::cout << "  注释: " << nlohmann::json(alert.getAnnotations()).dump(2) << std::endl;
        std::cout << "  开始时间: " << alert.getStartsAt() << std::endl;
        std::cout << "  结束时间: " << alert.getEndsAt() << std::endl;
        std::cout << "  创建时间: " << alert.getCreatedAt() << std::endl;
        std::cout << "  更新时间: " << alert.getUpdatedAt() << std::endl;
        std::cout << std::endl;
    }
}

} // namespace alertv2
} // namespace yw
