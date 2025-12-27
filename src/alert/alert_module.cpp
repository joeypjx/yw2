// ============================================================================
// 文件功能描述：
// 告警模块（AlertModule）的实现文件，是告警系统的核心适配器层。
// 主要功能包括：
// 1. 模块适配器实现：AlertModuleAdapter类实现了IAlertModule接口，统一封装告警系统的各个组件
// 2. 告警引擎管理：管理AlertEngine的生命周期，负责启动和停止定期评估告警规则
// 3. 告警规则管理：通过AlertRuleService提供规则的增删改查功能
// 4. 告警查询服务：通过AlertQueryService提供按状态、过滤条件等查询告警事件的功能
// 5. 告警创建工厂：通过AlertCreationFactory创建特定类型的告警（如板卡类型变化告警）
// 6. 工厂模式实现：AlertFactory提供createAlertModule静态方法，负责创建和组装所有告警组件
// 7. 数据持久化：集成数据库仓库层，实现告警规则和告警事件的持久化存储
// ============================================================================

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

// AlertEngine的适配器，实现IAlertModule接口
// 将AlertEngine、AlertRuleService、AlertQueryService和AlertCreationFactory的功能统一封装
class AlertModuleAdapter : public IAlertModule {
public:
    // 构造函数，初始化各个服务组件
    // engine: 告警引擎实例
    // ruleService: 告警规则服务实例
    // queryService: 告警查询服务实例
    // creationFactory: 告警创建工厂实例
    AlertModuleAdapter(std::shared_ptr<AlertEngine> engine,
                      std::shared_ptr<AlertRuleService> ruleService,
                      std::shared_ptr<AlertQueryService> queryService,
                      std::shared_ptr<AlertCreationFactory> creationFactory)
        : engine_(std::move(engine)),
          ruleService_(std::move(ruleService)),
          queryService_(std::move(queryService)),
          creationFactory_(std::move(creationFactory)) {}
    
    // 析构函数，自动停止告警引擎
    ~AlertModuleAdapter() override {
        stop();
    }

    //-------------------------------------------------------------------------
    // 生命周期管理
    //-------------------------------------------------------------------------
    
    // 启动告警引擎，开始定期评估告警规则
    // intervalSeconds: 评估间隔（秒），默认5秒
    void start(int intervalSeconds = 5) override {
        if (engine_) {
            engine_->start(intervalSeconds);
        }
    }
    
    // 停止告警引擎
    void stop() override {
        if (engine_) {
            engine_->stop();
        }
    }
    
    // 检查告警引擎是否正在运行
    // 返回: 如果引擎实例存在返回true，否则返回false
    bool isRunning() const override {
        // AlertEngine 没有暴露 isRunning，暂时返回 true
        // TODO: 添加 AlertEngine::isRunning() 方法
        return engine_ != nullptr;
    }

    //-------------------------------------------------------------------------
    // 告警规则管理
    //-------------------------------------------------------------------------
    
    // 添加告警规则
    // ruleJson: 告警规则JSON对象
    // 返回: 添加成功返回true，失败返回false
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
    
    // 更新告警规则
    // ruleJson: 告警规则JSON对象（必须包含id字段）
    // 返回: 更新成功返回true，失败返回false
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
    
    // 删除告警规则
    // ruleId: 告警规则ID
    // 返回: 删除成功返回true，失败返回false
    bool deleteAlertRule(const std::string& ruleId) override {
        if (!ruleService_) return false;
        return ruleService_->deleteAlertRule(ruleId);
    }
    
    // 根据ID获取告警规则
    // ruleId: 告警规则ID
    // 返回: 告警规则JSON对象，不存在时返回空JSON对象
    nlohmann::json getAlertRuleById(const std::string& ruleId) override {
        if (!ruleService_) return nlohmann::json();
        auto rule = ruleService_->getAlertRuleById(ruleId);
        if (!rule) return nlohmann::json();
        return rule->toJson();
    }
    
    // 获取所有告警规则
    // 返回: 告警规则JSON数组
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
    
    // 将告警列表转换为JSON数组
    // alerts: 告警事件列表
    // 返回: 告警事件JSON数组
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

// 创建告警模块实例
// dbConnInfo: PostgreSQL数据库连接字符串
// 返回: 告警模块实例，失败返回nullptr
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

