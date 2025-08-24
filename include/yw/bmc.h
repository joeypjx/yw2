#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace yw {
namespace bmc {

struct BMCSensorRow; // 前置声明

class IBMCModule {
public:
    virtual ~IBMCModule() = default;
    virtual std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration) const = 0;
};

class BMCFactory {
public:
    static std::shared_ptr<IBMCModule> getBMCModule();
};

} // namespace bmc
} // namespace yw