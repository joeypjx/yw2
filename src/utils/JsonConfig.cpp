#include "yw/JsonConfig.h"

#include <fstream>

namespace yw {
namespace utils {

JsonConfig& JsonConfig::instance() {
    static JsonConfig cfg;
    return cfg;
}

bool JsonConfig::loadFromFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    try {
        nlohmann::json j; ifs >> j; data_ = std::move(j);
        return true;
    } catch (...) {
        data_.clear();
        return false;
    }
}

} // namespace utils
} // namespace yw


