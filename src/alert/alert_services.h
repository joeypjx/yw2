#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

#include "yw/alert_model.h"

namespace yw {
namespace alert {

struct EvaluationPoint {
    LabelSet        labels;     // 实例标签（如 host_ip 等）
    bool            matched;    // 是否命中
    double          value;      // 判定值（可选语义）
    nlohmann::json  context;    // 上下文（便于事件描述）
};

class ITimeseriesProvider {
public:
    virtual ~ITimeseriesProvider() = default;
    // 执行表达式（SQL/DSL），按选择器与窗口返回上下文结果。
    virtual nlohmann::json evaluate(const std::string& expression,
                                    const LabelSet& selector,
                                    const std::string& window) = 0;
};

class IRuleRepository {
public:
    virtual ~IRuleRepository() = default;
    virtual std::vector<Rule> listRules() const = 0;
    virtual std::optional<Rule> getRule(const std::string& id) const = 0;
    virtual bool upsertRule(const Rule& rule) = 0;
    virtual bool deleteRule(const std::string& id) = 0;
};

class IAlertRepository {
public:
    virtual ~IAlertRepository() = default;
    virtual std::optional<AlertState> getState(const std::string& fingerprint) const = 0;
    virtual bool upsertState(const AlertState& state) = 0;
    virtual std::vector<AlertState> listActive(const LabelSet& matcher) const = 0;
};

class IEventRepository {
public:
    virtual ~IEventRepository() = default;
    virtual bool append(const AlertEvent& event) = 0;
    virtual std::vector<AlertEvent> query(const std::string& duration) const = 0;
};

class IFingerprintGenerator {
public:
    virtual ~IFingerprintGenerator() = default;
    virtual std::string generate(const std::string& rule_id, const LabelSet& labels) const = 0;
};

class IAlertEvaluator {
public:
    virtual ~IAlertEvaluator() = default;
    virtual std::vector<EvaluationPoint> evaluate(const Rule& rule, std::int64_t now_ms) = 0;
};

class IAlertStateManager {
public:
    virtual ~IAlertStateManager() = default;
    virtual std::vector<AlertEvent> apply(const Rule& rule,
                                          const std::vector<EvaluationPoint>& points,
                                          std::int64_t now_ms) = 0;
    virtual std::vector<AlertState> listActive(const LabelSet& matcher) const = 0;
    virtual bool ack(const std::string& fingerprint, const std::string& user, const std::string& comment) = 0;
};

class IScheduler {
public:
    using Task = std::function<void()>;
    virtual ~IScheduler() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void registerTask(const std::string& id, std::int64_t interval_ms, Task task) = 0;
    virtual void unregisterTask(const std::string& id) = 0;
};

class IAggregateManager {
public:
    virtual ~IAggregateManager() = default;
    virtual bool ensureInitialized() = 0; // 初始化/迁移连续聚合
};

} // namespace alert
} // namespace yw


