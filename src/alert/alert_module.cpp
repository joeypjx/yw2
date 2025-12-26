#include "alert/alert.h"
#include "application/alert_engine.h"
#include "application/alert_rule_service.h"
#include "application/alert_query_service.h"
#include "application/alert_creation_factory.h"
#include "infrastructure/database_query_interface.h"
#include "infrastructure/alert_rule_repository.h"
#include "infrastructure/alert_event_repository.h"
#include <spdlog/spdlog.h>

namespace yw {
namespace alert {

/**
 * @brief AlertEngine 的适配器，实现 IAlertModule 接口
 */
class AlertModuleAdapter : public IAlertModule {
public:
    AlertModuleAdapter(std::shared_ptr<AlertEngine> engine,
                      std::shared_ptr<AlertRuleService> ruleService,
                      std::shared_ptr<AlertQueryService> queryService,
                      std::shared_ptr<AlertCreationFactory> creationFactory)
        : engine_(std::move(engine)),
          ruleService_(std::move(ruleService)),
          queryService_(std::move(queryService)),
          creationFactory_(std::move(creationFactory)) {}
    
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
        if (!ruleService_) return false;
        try {
            AlertRule rule = AlertRule::fromJson(ruleJson);
            return ruleService_->addAlertRule(rule);
        } catch (const std::exception& e) {
            spdlog::error("Failed to add alert rule: {}", e.what());
            return false;
        }
    }
    
    bool updateAlertRule(const nlohmann::json& ruleJson) override {
        if (!ruleService_) return false;
        try {
            AlertRule rule = AlertRule::fromJson(ruleJson);
            return ruleService_->updateAlertRule(rule);
        } catch (const std::exception& e) {
            spdlog::error("Failed to update alert rule: {}", e.what());
            return false;
        }
    }
    
    bool deleteAlertRule(const std::string& ruleId) override {
        if (!ruleService_) return false;
        return ruleService_->deleteAlertRule(ruleId);
    }
    
    nlohmann::json getAlertRuleById(const std::string& ruleId) override {
        if (!ruleService_) return nlohmann::json();
        auto rule = ruleService_->getAlertRuleById(ruleId);
        if (!rule) return nlohmann::json();
        return rule->toJson();
    }
    
    nlohmann::json getAllAlertRules() const override {
        if (!ruleService_) return nlohmann::json::array();
        auto rules = ruleService_->getAllAlertRules();
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
        if (!queryService_) return nlohmann::json::array();
        auto alerts = queryService_->getAlertsByStatus(status);
        return alertsToJson(alerts);
    }
    
    nlohmann::json getAlertsByFilters(const AlertFilters& filters) override {
        if (!queryService_) return nlohmann::json::array();
        auto alerts = queryService_->getAlertsByFilters(filters);
        return alertsToJson(alerts);
    }
    
    nlohmann::json getAlertById(const std::string& alertId) override {
        if (!queryService_) return nlohmann::json();
        auto alert = queryService_->getAlertById(alertId);
        if (!alert) return nlohmann::json();
        return alert->toJson();
    }
    
    nlohmann::json getAlertsExceptPending() override {
        if (!queryService_) return nlohmann::json::array();
        auto alerts = queryService_->getAlertsExceptPending();
        return alertsToJson(alerts);
    }

    //-------------------------------------------------------------------------
    // 统计信息
    //-------------------------------------------------------------------------
    
    size_t getAlertCount() override {
        if (!queryService_) return 0;
        return queryService_->getAlertCount();
    }

    //-------------------------------------------------------------------------
    // 回调设置
    //-------------------------------------------------------------------------
    
    void setPushCallback(std::function<void(const nlohmann::json&)> callback) override {
        if (!engine_ || !creationFactory_) return;
        // 包装回调，将 AlertEvent 转换为 JSON
        auto wrappedCallback = [callback](const AlertEvent& alert) {
            if (callback) {
                callback(alert.toJson());
            }
        };
        // 同时设置到 AlertEngine 和 AlertCreationFactory
        engine_->setPushCallback(wrappedCallback);
        creationFactory_->setPushCallback(wrappedCallback);
    }
    
    //-------------------------------------------------------------------------
    // 告警创建
    //-------------------------------------------------------------------------
    
    nlohmann::json createBoardTypeChangeAlert(
        int box_id, int slot_id,
        const std::string& cached_board_type,
        const std::string& new_board_type) override {
        
        if (!creationFactory_) return nlohmann::json();
        auto alert = creationFactory_->createBoardTypeChangeAlert(
            box_id, slot_id, cached_board_type, new_board_type);
        if (!alert) return nlohmann::json();
        return alert->toJson();
    }

private:
    std::shared_ptr<AlertEngine> engine_;  // 仅用于生命周期管理和评估引擎
    std::shared_ptr<AlertRuleService> ruleService_;
    std::shared_ptr<AlertQueryService> queryService_;
    std::shared_ptr<AlertCreationFactory> creationFactory_;
    
    /**
     * @brief 将告警列表转换为 JSON 数组
     */
    static nlohmann::json alertsToJson(const std::vector<AlertEvent>& alerts) {
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
    const std::string& dbConnInfo) {
    
    try {
        // 创建数据库连接
        auto dbInterface = std::make_shared<PostgreSQLQueryInterface>(dbConnInfo);
        
        // 创建 Repository
        auto alertRuleRepo = std::make_shared<DatabaseAlertRuleRepository>(dbInterface);
        auto alertRepo = std::make_shared<DatabaseAlertEventRepository>(dbInterface);
        
        // 创建服务类
        auto ruleService = std::make_shared<AlertRuleService>(alertRuleRepo);
        auto queryService = std::make_shared<AlertQueryService>(alertRepo);
        auto creationFactory = std::make_shared<AlertCreationFactory>(alertRepo, dbInterface);
        
        // 创建 AlertEngine（仅用于评估引擎功能）
        // 传入共享的 ruleService，确保 AlertEngine 和 AlertModuleAdapter 使用同一个实例
        auto engine = std::make_shared<AlertEngine>(
            dbInterface, alertRepo, ruleService);
        
        // 返回适配器，直接使用服务类
        return std::make_shared<AlertModuleAdapter>(
            engine, ruleService, queryService, creationFactory);
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create AlertModule: {}", e.what());
        return nullptr;
    }
}

} // namespace alert
} // namespace yw

