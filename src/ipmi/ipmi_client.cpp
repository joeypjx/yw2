#include "ipmi_client.h"

#include <freeipmi/api/ipmi-api.h>
#include <freeipmi/spec/ipmi-privilege-level-spec.h>
#include <freeipmi/interface/ipmi-rmcpplus-interface.h>

#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace yw
{
    namespace ipmi
    {

        // IPMI客户端内部实现结构
        struct IpmiClient::Impl
        {
            ipmi_ctx_t ctx = nullptr;  // FreeIPMI上下文
            Options opt;                // IPMI连接选项
        };

        // 获取最后一次IPMI操作的错误信息
        static std::string lastError(ipmi_ctx_t ctx, int /* err */)
        {
            int errnum = ipmi_ctx_errnum(ctx);
            const char *s = ipmi_ctx_errormsg(ctx);
            if (s && *s)
                return std::string(s);
            return "libfreeipmi error code " + std::to_string(errnum);
        }

        // IPMI客户端构造函数，初始化FreeIPMI上下文并建立连接
        // options: IPMI连接选项（主机名、用户名、密码等）
        IpmiClient::IpmiClient(const Options &options)
        {
            impl_ = new Impl();
            impl_->opt = options;
            impl_->ctx = ipmi_ctx_create();
            if (!impl_->ctx)
            {
                throw std::runtime_error("ipmi_ctx_create failed");
            }

            // IPMI会话密钥（RMCP+认证时使用，这里不使用）
            const unsigned char *kg = nullptr;
            unsigned int kg_len = 0;

            // 使用FreeIPMI库建立RMCP+ 2.0协议的IPMI连接
            // 支持加密认证和会话管理
            int rc = ipmi_ctx_open_outofband_2_0(
                impl_->ctx,  // IPMI上下文
                options.hostname.c_str(),  // BMC主机名或IP地址
                options.username.c_str(),  // IPMI用户名
                options.password.c_str(),  // IPMI密码
                kg,  // 会话密钥（nullptr表示不使用）
                kg_len,  // 会话密钥长度
                options.privilegeLevel,  // 权限级别（如管理员、操作员等）
                options.cipherSuiteId,  // 加密套件ID（用于选择加密算法）
                options.sessionTimeoutMs,  // 会话超时时间（毫秒）
                options.retransmissionTimeoutMs,  // 重传超时时间（毫秒）
                IPMI_WORKAROUND_FLAGS_OUTOFBAND_2_0_AUTHENTICATION_CAPABILITIES,  // 工作区标志（处理某些BMC的兼容性问题）
                IPMI_FLAGS_DEFAULT);  // 默认标志

            // 连接失败时，先获取错误信息再清理资源
            if (rc < 0)
            {
                // 首先获取错误信息（必须在销毁上下文之前）
                std::string err = lastError(impl_->ctx, rc);

                // 然后再销毁和清理资源
                ipmi_ctx_destroy(impl_->ctx);
                impl_->ctx = nullptr;
                delete impl_;
                impl_ = nullptr;
                throw std::runtime_error(err);
            }
        }

        // IPMI客户端析构函数，关闭连接并清理资源
        IpmiClient::~IpmiClient()
        {
            if (impl_)
            {
                if (impl_->ctx)
                {
                    ipmi_ctx_close(impl_->ctx);
                    ipmi_ctx_destroy(impl_->ctx);
                    impl_->ctx = nullptr;
                }
                delete impl_;
                impl_ = nullptr;
            }
        }

        // 发送原始IPMI命令
        // inputs: 输入字节数组（IPMI命令数据）
        // outputs: 输出参数，接收IPMI响应数据
        // errorMessage: 输出参数，错误信息
        // 返回: 成功返回true，失败返回false
        bool IpmiClient::sendRaw(const std::vector<uint8_t> &inputs, std::vector<uint8_t> &outputs, std::string &errorMessage)
        {
            if (!impl_ || !impl_->ctx)
            {
                errorMessage = "ctx not initialized";
                return false;
            }

            if (inputs.size() < 2)
            {
                errorMessage = "inputs must contain at least netfn and cmd";
                return false;
            }

            // 解析输入数据：第一个字节是网络功能号（netfn），第二个字节是命令码（cmd）
            uint8_t netfn = inputs[0];  // 网络功能号（如0x06表示应用功能）
            uint8_t cmd = inputs[1];  // 命令码（具体操作）

            // 提取命令数据（从第三个字节开始）
            const void *rq_data = nullptr;
            unsigned int rq_len = 0;
            if (inputs.size() > 2)
            {
                rq_data = &inputs[2];  // 命令数据起始地址
                rq_len = static_cast<unsigned int>(inputs.size() - 2);  // 命令数据长度
            }

            // 构造IPMI原始请求缓冲区：第一个字节必须是命令码
            // FreeIPMI的ipmi_cmd_raw_ipmb函数要求请求缓冲区以命令码开头
            std::vector<uint8_t> raw_rq;
            raw_rq.reserve(1 + rq_len);
            raw_rq.push_back(cmd);  // 命令码作为第一个字节
            if (rq_len)
            {
                // 追加命令数据
                raw_rq.insert(raw_rq.end(), static_cast<const uint8_t *>(rq_data), static_cast<const uint8_t *>(rq_data) + rq_len);
            }

            // 获取IPMI通道、目标地址和LUN（逻辑单元号）
            uint8_t channel = impl_->opt.channel;  // IPMI通道号（通常为0x0E表示系统接口）
            uint8_t rs_addr = impl_->opt.targetAddress;  // 目标设备地址（BMC地址）
            uint8_t lun = impl_->opt.lun;  // 逻辑单元号（通常为0）

            // 分配响应缓冲区（IPMI响应最大256字节）
            uint8_t resp_buf[256];
            std::memset(resp_buf, 0, sizeof(resp_buf));

            // 使用IPMB（Intelligent Platform Management Bus）桥接方式发送原始IPMI命令
            // 这种方式通过IPMB总线发送命令，等价于ipmitool的-b/-t选项
            int rc = ipmi_cmd_raw_ipmb(
                impl_->ctx,  // IPMI上下文
                channel,  // 通道号
                rs_addr,  // 目标地址
                lun,  // 逻辑单元号
                netfn,  // 网络功能号
                raw_rq.data(),  // 请求数据（以命令码开头）
                static_cast<unsigned int>(raw_rq.size()),  // 请求数据长度
                resp_buf,  // 响应缓冲区
                sizeof(resp_buf));  // 响应缓冲区大小

            // 检查响应长度（至少应包含1字节的完成码）
            if (rc < 1)
            {
                errorMessage = "invalid response length";
                return false;
            }

            // 检查完成码（Completion Code），0x00表示成功
            uint8_t ccode = resp_buf[0];
            if (ccode != 0x00)
            {
                errorMessage = "IPMI command failed, completion code: " + std::to_string(ccode);
                return false;
            }

            // 去掉完成码（第一个字节），返回真正的响应数据（payload）
            outputs.assign(resp_buf + 1, resp_buf + rc);

            return true;
        }

    } // namespace ipmi
} // namespace yw