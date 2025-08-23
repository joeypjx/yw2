#pragma once

#include <string>
#include "monitor_model.h"

namespace yw {
namespace monitor {

class ResourceRepository {
public:
    explicit ResourceRepository(const std::string& conninfo);

    // 将一次 /resource 的 data 写入 TimescaleDB（多表，单事务）
    void save(const Resource& data);

private:
    std::string conninfo_;
};

} // namespace monitor
} // namespace yw


