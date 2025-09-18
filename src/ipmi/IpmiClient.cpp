#include "IpmiClient.h"

#include <freeipmi/api/ipmi-api.h>
#include <freeipmi/spec/ipmi-privilege-level-spec.h>
#include <freeipmi/interface/ipmi-rmcpplus-interface.h>

#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace yw {
namespace ipmi {

struct IpmiClient::Impl {
    ipmi_ctx_t ctx = nullptr;
    Options opt;
};

static std::string lastError(ipmi_ctx_t ctx, int err)
{
    if (!ctx) return "libfreeipmi: ctx null";
    const char *s = ipmi_ctx_errormsg(ctx);
    if (s && *s) return std::string(s);
    return std::string("libfreeipmi error ") + std::to_string(err);
}

IpmiClient::IpmiClient(const Options &options)
{
    impl_ = new Impl();
    impl_->opt = options;
    impl_->ctx = ipmi_ctx_create();
    if (!impl_->ctx) {
        throw std::runtime_error("ipmi_ctx_create failed");
    }

    const unsigned char *kg = nullptr;
    unsigned int kg_len = 0;

    int rc = ipmi_ctx_open_outofband_2_0(
        impl_->ctx,
        options.hostname.c_str(),
        options.username.c_str(),
        options.password.c_str(),
        kg,
        kg_len,
        options.privilegeLevel,
        options.cipherSuiteId,
        options.sessionTimeoutMs,
        options.retransmissionTimeoutMs,
        IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_AUTHENTICATION_CAPABILITIES,
        IPMI_FLAGS_DEFAULT
    );

    if (rc < 0) {
        std::string err = lastError(impl_->ctx, rc);
        ipmi_ctx_destroy(impl_->ctx);
        impl_->ctx = nullptr;
        delete impl_;
        impl_ = nullptr;
        throw std::runtime_error(err);
    }
}

IpmiClient::~IpmiClient()
{
    if (impl_) {
        if (impl_->ctx) {
            ipmi_ctx_close(impl_->ctx);
            ipmi_ctx_destroy(impl_->ctx);
        }
        delete impl_;
        impl_ = nullptr;
    }
}

bool IpmiClient::sendRaw(const std::vector<uint8_t> &inputs, std::vector<uint8_t> &outputs, std::string &errorMessage)
{
    if (!impl_ || !impl_->ctx) {
        errorMessage = "ctx not initialized";
        return false;
    }

    if (inputs.size() < 2) {
        errorMessage = "inputs must contain at least netfn and cmd";
        return false;
    }

    uint8_t netfn = inputs[0];
    uint8_t cmd   = inputs[1];

    const void *rq_data = nullptr;
    unsigned int rq_len = 0;
    if (inputs.size() > 2) {
        rq_data = &inputs[2];
        rq_len = static_cast<unsigned int>(inputs.size() - 2);
    }

    // 构造请求缓冲区: 第一个字节需要是命令码
    std::vector<uint8_t> raw_rq;
    raw_rq.reserve(1 + rq_len);
    raw_rq.push_back(cmd);
    if (rq_len) {
        raw_rq.insert(raw_rq.end(), static_cast<const uint8_t*>(rq_data), static_cast<const uint8_t*>(rq_data) + rq_len);
    }

    uint8_t channel = impl_->opt.channel;
    uint8_t rs_addr = impl_->opt.targetAddress;

    uint8_t lun = impl_->opt.lun;

    // 响应缓冲区
    uint8_t resp_buf[256];
    std::memset(resp_buf, 0, sizeof(resp_buf));

    // 使用 bridged raw，等价于 -b / -t 通过 IPMB
    int rc = ipmi_cmd_raw_ipmb(
        impl_->ctx,
        channel,
        rs_addr,
        lun,
        netfn,
        raw_rq.data(),
        static_cast<unsigned int>(raw_rq.size()),
        resp_buf,
        sizeof(resp_buf)
    );

    if (rc < 0) {
        errorMessage = lastError(impl_->ctx, rc);
        return false;
    }

    // rc 为写入到 resp_buf 的长度
    outputs.assign(resp_buf, resp_buf + rc);
    return true;
}

} // namespace ipmi
} // namespace yw