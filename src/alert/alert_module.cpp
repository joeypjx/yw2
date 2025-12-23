#include "alert/alert.h"
#include "application/alert_engine.h"
#include "infrastructure/database_query_interface.h"
#include "infrastructure/alert_rule_repository.h"
#include "infrastructure/alert_repository.h"
#include <spdlog/spdlog.h>

namespace yw {
namespace alert {

/**
 * @brief AlertEngine 的适配器，实现 IAlertModule 接口
 */
class AlertModuleAdapter : public IAlertModule {
public:
    AlertModuleAdapter(std::shared_ptr<AlertEngine> engine)
        : engine_(std::move(engine)) {}
    
    ~AlertModuleAdapter() override {
        stop();
    }

    //-------------------------------------------------------------------------
    // 生命周期管理
    //-------------------------------------------------------------------------
    
    void start(int intervalSeconds = 5) override {
        if (engine_) {
            engine_->start(intervalSeconds);
        }
    }
    
    void stop() override {
        if (engine_) {
            engine_->stop();
        }
    }
    
    bool isRunning() const override {
        // AlertEngine 没有暴露 isRunning，暂时返回 true
        // TODO: 添加 AlertEngine::isRunning() 方法
        return engine_ != nullptr;
    }

    //-------------------------------------------------------------------------
    // 告警规则管理
    //-------------------------------------------------------------------------
    
    bool addAlertRule(const nlohmann::json& ruleJson) override {
        if (!engine_) return false;
        try {
            AlertRule rule = AlertRule::fromJson(ruleJson);
            return engine_->addAlertRule(rule);
        } catch (const std::exception& e) {
            spdlog::error("Failed to add alert rule: {}", e.what());
            return false;
        }
    }
    
    bool updateAlertRule(const nlohmann::json& ruleJson) override {
        if (!engine_) return false;
        try {
            AlertRule rule = AlertRule::fromJson(ruleJson);
            return engine_->updateAlertRule(rule);
        } catch (const std::exception& e) {
            spdlog::error("Failed to update alert rule: {}", e.what());
            return false;
        }
    }
    
    bool deleteAlertRule(const std::string& ruleId) override {
        if (!engine_) return false;
        return engine_->deleteAlertRule(ruleId);
    }
    
    nlohmann::json getAlertRuleById(const std::string& ruleId) override {
        if (!engine_) return nlohmann::json();
        auto rule = engine_->getAlertRuleById(ruleId);
        if (!rule) return nlohmann::json();
        return rule->toJson();
    }
    
    nlohmann::json getAllAlertRules() const override {
        if (!engine_) return nlohmann::json::array();
        auto rules = engine_->getAllAlertRules();
        nlohmann::json result = nlohmann::json::array();
        for (const auto& rule : rules) {
            result.push_back(rule.toJson());
        }
        return result;
    }

    //-------------------------------------------------------------------------
    // 告警查询
    //-------------------------------------------------------------------------
    
    nlohmann::json getAlertsByStatus(const std::string& status) override {
        if (!engine_) return nlohmann::json::array();
        auto alerts = engine_->getAlertsByStatus(status);
        return alertsToJson(alerts);
    }
    
    nlohmann::json getAlertsByFilters(const AlertFilters& filters) override {
        if (!engine_) return nlohmann::json::array();
        auto alerts = engine_->getAlertsByFilters(filters);
        return alertsToJson(alerts);
    }
    
    nlohmann::json getAlertById(const std::string& alertId) override {
        if (!engine_) return nlohmann::json();
        auto alert = engine_->getAlertById(alertId);
        if (!alert) return nlohmann::json();
        return alert->toJson();
    }
    
    nlohmann::json getAlertsExceptPending() override {
        if (!engine_) return nlohmann::json::array();
        auto alerts = engine_->getAlertsExceptPending();
        return alertsToJson(alerts);
    }

    //-------------------------------------------------------------------------
    // 统计信息
    //-------------------------------------------------------------------------
    
    size_t getAlertCount() override {
        if (!engine_) return 0;
        return engine_->getAlertCount();
    }

    //-------------------------------------------------------------------------
    // 回调设置
    //-------------------------------------------------------------------------
    
    void setPushCallback(std::function<void(const nlohmann::json&)> callback) override {
        if (!engine_) return;
        // 包装回调，将 Alert 转换为 JSON
        engine_->setPushCallback([callback](const Alert& alert) {
            if (callback) {
                callback(alert.toJson());
            }
        });
    }
    
    //-------------------------------------------------------------------------
    // 告警创建
    //-------------------------------------------------------------------------
    
    nlohmann::json createBoardTypeChangeAlert(
        int box_id, int slot_id,
        const std::string& cached_board_type,
        const std::string& new_board_type) override {
        
        if (!engine_) return nlohmann::json();
        auto alert = engine_->createBoardTypeChangeAlert(
            box_id, slot_id, cached_board_type, new_board_type);
        if (!alert) return nlohmann::json();
        return alert->toJson();
    }

private:
    std::shared_ptr<AlertEngine> engine_;
    
    /**
     * @brief 将告警列表转换为 JSON 数组
     */
    static nlohmann::json alertsToJson(const std::vector<Alert>& alerts) {
        nlohmann::json result = nlohmann::json::array();
        for (const auto& alert : alerts) {
            result.push_back(alert.toJson());
        }
        return result;
    }
};

//=============================================================================
// AlertFactory 实现
//=============================================================================

std::shared_ptr<IAlertModule> AlertFactory::createAlertModule(
    const std::string& dbConnInfo,
    node::INodeModule* nodeModule) {
    
    try {
        // 创建数据库连接
        auto dbInterface = std::make_shared<PostgreSQLQueryInterface>(dbConnInfo);
        
        // 创建 Repository
        auto alertRuleRepo = std::make_shared<DatabaseAlertRuleRepository>(dbInterface);
        auto alertRepo = std::make_shared<DatabaseAlertRepository>(dbInterface);
        
        // 创建 AlertEngine
        auto engine = std::make_shared<AlertEngine>(
            dbInterface, alertRuleRepo, alertRepo, nodeModule);
        
        // 返回适配器
        return std::make_shared<AlertModuleAdapter>(engine);
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create AlertModule: {}", e.what());
        return nullptr;
    }
}

} // namespace alert
} // namespace yw

