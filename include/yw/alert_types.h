#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace yw {
namespace alert {

using LabelSet = std::unordered_map<std::string, std::string>;

enum class Severity { Info, Warn, Critical };
enum class AlertStatus { Inactive, Pending, Firing, Resolved };

NLOHMANN_JSON_SERIALIZE_ENUM(Severity, {
    {Severity::Info,      "提示"},
    {Severity::Warn,      "一般"},
    {Severity::Critical,  "严重"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(AlertStatus, {
    {AlertStatus::Inactive, "inactive"},
    {AlertStatus::Pending,  "pending"},
    {AlertStatus::Firing,   "firing"},
    {AlertStatus::Resolved, "resolved"}
})

} // namespace alert
} // namespace yw




