#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <optional>

#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

#include "yw/alert.h"
#include "AlertServices.h"

namespace yw {
namespace alert {

class AlertManager : public IAlertModule {
public:
    AlertManager();
    ~AlertManager() override;

    // 规则管理
    std::vector<Rule> listRules() const override;
    std::optional<Rule> getRule(const std::string& id) const override;
    bool upsertRule(const Rule& rule) override;
    bool deleteRule(const std::string& id) override;

    // 告警查询与操作
    std::vector<AlertState> listActiveAlerts(const LabelSet& matcher) const override;
    std::vector<AlertEvent> queryEvents(const std::string& duration) const override;
    bool ackAlert(const std::string& fingerprint, const std::string& user, const std::string& comment) override;

private:
    std::shared_ptr<pqxx::connection> conn_;
    mutable std::mutex                mu_;

    // 内部服务聚合
    std::shared_ptr<IRuleRepository>       rule_repo_;
    std::shared_ptr<IAlertRepository>      alert_repo_;
    std::shared_ptr<IEventRepository>      event_repo_;
    std::shared_ptr<IFingerprintGenerator> fp_;
    std::shared_ptr<ITimeseriesProvider>   ts_;
    std::shared_ptr<IAlertEvaluator>       evaluator_;
    std::shared_ptr<IAlertStateManager>    state_manager_;
    std::shared_ptr<IScheduler>            scheduler_;
};

} // namespace alert
} // namespace yw


