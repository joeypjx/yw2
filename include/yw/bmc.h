#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <optional>

#include "yw/bmc_model.h"

namespace yw {
namespace bmc {

struct BMCSensorRow; // 前置声明

struct UdpInfo;

class IBMCModule {
public:
    virtual ~IBMCModule() = default;
    virtual std::unordered_map<std::string, std::vector<BMCSensorRow>> queryBMCSensor(
        const std::string& host_ip,
        const std::string& duration) const = 0;

    virtual std::optional<UdpInfo> getBoxBMC(int box_id) const = 0;

    // 返回所有 box 的最新 BMC UdpInfo
    virtual std::vector<UdpInfo> getAllBoxBMC() const = 0;
};

class BMCFactory {
public:
    static std::shared_ptr<IBMCModule> getBMCModule();
};

} // namespace bmc
} // namespace yw