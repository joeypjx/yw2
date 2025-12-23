#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace yw {
namespace web {

struct AlertEventAnnotations {
    std::string description;
    std::string summary;
};

struct UserAlertEventView {
    AlertEventAnnotations annotations;
    std::string           created_at;   // YYYY-MM-DD HH:MM:SS
    std::string           ends_at;      // YYYY-MM-DD HH:MM:SS 或空串
    std::string           fingerprint;
    std::string           id;           // 暂时使用 fingerprint 作为 id
    std::map<std::string, std::string> labels;
    std::string           starts_at;    // YYYY-MM-DD HH:MM:SS
    std::string           status;       // inactive/pending/firing/resolved
    std::string           updated_at;   // YYYY-MM-DD HH:MM:SS
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEventAnnotations,
    description, summary)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserAlertEventView,
    annotations, created_at, ends_at, fingerprint, id, labels, starts_at, status, updated_at)

} // namespace web
} // namespace yw

