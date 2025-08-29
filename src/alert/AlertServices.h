#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <optional>
#include <thread>
#include <atomic>

#include <pqxx/pqxx>

#include "alert_services.h"

namespace yw {
namespace alert {

class SimpleFingerprintGenerator : public IFingerprintGenerator {
public:
    std::string generate(const std::string& rule_id, const LabelSet& labels) const override;
};

class SimpleTimeseriesProvider : public ITimeseriesProvider {
public:
    explicit SimpleTimeseriesProvider(std::shared_ptr<pqxx::connection> conn);
    nlohmann::json evaluate(const std::string& expression,
                            const LabelSet& selector,
                            const std::string& window) override;
private:
    std::shared_ptr<pqxx::connection> conn_;
};

class MemoryAlertRepository : public IAlertRepository {
public:
    std::optional<AlertState> getState(const std::string& fingerprint) const override;
    bool upsertState(const AlertState& state) override;
    std::vector<AlertState> listActive(const LabelSet& matcher) const override;
private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, AlertState> states_;
};

class MemoryEventRepository : public IEventRepository {
public:
    bool append(const AlertEvent& event) override;
    std::vector<AlertEvent> query(const std::string& duration) const override;
    std::size_t countByStatus(AlertStatus status) const override;
private:
    mutable std::mutex mu_;
    std::vector<AlertEvent> events_;
};

class MemoryRuleRepository : public IRuleRepository {
public:
    std::vector<Rule> listRules() const override;
    std::optional<Rule> getRule(const std::string& id) const override;
    bool upsertRule(const Rule& rule) override;
    bool deleteRule(const std::string& id) override;
private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, Rule> rules_;
};

class BasicAlertEvaluator : public IAlertEvaluator {
public:
    explicit BasicAlertEvaluator(std::shared_ptr<ITimeseriesProvider> ts);
    std::vector<EvaluationPoint> evaluate(const Rule& rule, std::int64_t now_ms) override;
private:
    std::shared_ptr<ITimeseriesProvider> ts_;
};

class BasicAlertStateManager : public IAlertStateManager {
public:
    BasicAlertStateManager(std::shared_ptr<IAlertRepository> repo,
                           std::shared_ptr<IEventRepository> event_repo,
                           std::shared_ptr<IFingerprintGenerator> fp);
    std::vector<AlertEvent> apply(const Rule& rule,
                                  const std::vector<EvaluationPoint>& points,
                                  std::int64_t now_ms) override;
    std::vector<AlertState> listActive(const LabelSet& matcher) const override;
    bool ack(const std::string& fingerprint, const std::string& user, const std::string& comment) override;
private:
    std::shared_ptr<IAlertRepository> repo_;
    std::shared_ptr<IEventRepository> event_repo_;
    std::shared_ptr<IFingerprintGenerator> fp_;
};

class BasicScheduler : public IScheduler {
public:
    BasicScheduler();
    ~BasicScheduler() override;
    void start() override;
    void stop() override;
    void registerTask(const std::string& id, std::int64_t interval_ms, Task task) override;
    void unregisterTask(const std::string& id) override;
private:
    struct TaskEntry { std::int64_t interval_ms; Task task; std::int64_t next_run_ms; };
    std::atomic<bool> running_{false};
    std::thread worker_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, TaskEntry> tasks_;
};

} // namespace alert
} // namespace yw


