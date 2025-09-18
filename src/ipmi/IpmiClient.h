#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "yw/ipmi.h"

namespace yw {
namespace ipmi {

class IpmiClient : public IIPMIModule {
public:
    explicit IpmiClient(const Options &options);
    ~IpmiClient() override;

    bool sendRaw(const std::vector<uint8_t> &inputs, std::vector<uint8_t> &outputs, std::string &errorMessage) override;

private:
    struct Impl;
    Impl *impl_;
};

} // namespace ipmi
} // namespace yw


