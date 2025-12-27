#include "resource_controller.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace yw {
namespace controller {

// 资源控制器构造函数
ResourceController::ResourceController() = default;

// 重置指定机箱的板卡（通过TCP发送控制命令）
IControllerModule::OperationResponse ResourceController::resetBoard(
    const std::string& target_ip,
    const std::vector<int>& slot_numbers,
    uint32_t req_id) {
    return executeOperation("RESET", target_ip, slot_numbers, req_id);
}

// 关闭指定机箱的板卡电源（通过TCP发送控制命令）
IControllerModule::OperationResponse ResourceController::powerOffChassisBoards(
    const std::string& target_ip,
    const std::vector<int>& slot_numbers,
    uint32_t req_id) {
    return executeOperation("POWOFF", target_ip, slot_numbers, req_id);
}

// 开启指定机箱的板卡电源（通过TCP发送控制命令）
IControllerModule::OperationResponse ResourceController::powerOnChassisBoards(
    const std::string& target_ip,
    const std::vector<int>& slot_numbers,
    uint32_t req_id) {
    return executeOperation("POWON", target_ip, slot_numbers, req_id);
}

// 执行板卡控制操作（重置/上电/下电）
// 通过TCP连接到目标IP的33000端口，发送二进制命令并等待响应
IControllerModule::OperationResponse ResourceController::executeOperation(
    const std::string& cmd,
    const std::string& target_ip,
    const std::vector<int>& slot_numbers,
    uint32_t req_id) {
    
    OperationResponse response;

    try {
        // 1) 使用TCP socket与目标机箱建立连接，进行请求-响应通信

        // 2) 构建操作模型（包含命令、目标IP、槽位列表、请求ID等）
        // 并将结构体转换为二进制数据准备发送
        ResourceController::OperationModel op_model = buildOperationModel(cmd, target_ip, slot_numbers, req_id);
        BinaryData binary_data(
            reinterpret_cast<const uint8_t*>(&op_model),
            reinterpret_cast<const uint8_t*>(&op_model) + sizeof(ResourceController::OperationModel)
        );

        spdlog::info("Executing chassis operation: {} to {}:{}; waiting response on same connection", 
                     cmd, target_ip, server_port_);
        spdlog::info("Sending binary data: {}", ResourceController::binaryToHex(binary_data));

        // 创建TCP socket（IPv4，流式套接字）
        int send_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (send_fd < 0) {
            response.result = IControllerModule::OperationResult::NETWORK_ERROR;
            response.message = "Failed to create send socket";
            return response;
        }

        // 设置socket为非阻塞模式，以便使用select实现连接超时控制
        int flags = ::fcntl(send_fd, F_GETFL, 0);
        if (flags >= 0) {
            ::fcntl(send_fd, F_SETFL, flags | O_NONBLOCK);
        }

        // 设置目标地址和端口（默认33000端口）
        sockaddr_in remote{};
        remote.sin_family = AF_INET;
        remote.sin_port = htons(static_cast<uint16_t>(server_port_));
        if (::inet_pton(AF_INET, target_ip.c_str(), &remote.sin_addr) != 1) {
            ::close(send_fd);
            response.result = IControllerModule::OperationResult::NETWORK_ERROR;
            response.message = "Invalid target IP";
            return response;
        }

        // 尝试连接目标，如果返回EINPROGRESS说明连接正在进行中
        int rc = ::connect(send_fd, reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
        if (rc < 0 && errno == EINPROGRESS) {
            // 使用select等待连接完成，设置超时时间
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(send_fd, &wfds);
            timeval ctv{};
            ctv.tv_sec = timeout_seconds_;
            ctv.tv_usec = 0;
            int sel = ::select(send_fd + 1, nullptr, &wfds, nullptr, &ctv);
            if (sel <= 0) {
                ::close(send_fd);
                response.result = (sel == 0) ? IControllerModule::OperationResult::TIMEOUT_ERROR : IControllerModule::OperationResult::NETWORK_ERROR;
                response.message = (sel == 0) ? "Connect timeout to target" : "Select error on connect";
                return response;
            }
            // 检查socket错误状态，确认连接是否成功
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (::getsockopt(send_fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0 || so_error != 0) {
                ::close(send_fd);
                response.result = IControllerModule::OperationResult::NETWORK_ERROR;
                response.message = "Connect failed to target";
                return response;
            }
        } else if (rc < 0) {
            // 连接失败（非EINPROGRESS错误）
            ::close(send_fd);
            response.result = IControllerModule::OperationResult::NETWORK_ERROR;
            response.message = "Connect error to target";
            return response;
        }

        // 连接建立后切回阻塞模式，以便SO_SNDTIMEO和SO_RCVTIMEO超时选项生效
        if (flags >= 0) {
            ::fcntl(send_fd, F_SETFL, flags & ~O_NONBLOCK);
        }

        // 设置发送超时时间，防止发送操作无限期阻塞
        timeval snd_to{};
        snd_to.tv_sec = timeout_seconds_;
        snd_to.tv_usec = 0;
        ::setsockopt(send_fd, SOL_SOCKET, SO_SNDTIMEO, &snd_to, sizeof(snd_to));

        // 循环发送完整报文，确保所有数据都被发送
        size_t total_sent = 0;
        const uint8_t* buf = binary_data.data();
        const size_t total_size = binary_data.size();
        while (total_sent < total_size) {
            ssize_t n = ::send(send_fd, reinterpret_cast<const char*>(buf + total_sent), static_cast<int>(total_size - total_sent), 0);
            if (n > 0) {
                // 成功发送部分数据，继续发送剩余部分
                total_sent += static_cast<size_t>(n);
            } else if (n < 0 && (errno == EINTR)) {
                // 被信号中断，重试
                continue;
            } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // 发送超时
                ::close(send_fd);
                response.result = IControllerModule::OperationResult::TIMEOUT_ERROR;
                response.message = "Send timeout to target";
                return response;
            } else {
                // 发送失败
                ::close(send_fd);
                response.result = IControllerModule::OperationResult::NETWORK_ERROR;
                response.message = "Failed to send data to target";
                return response;
            }
        }

        // 3) 在同一连接上接收响应（带超时控制）
        // 设置接收超时时间，防止接收操作无限期阻塞
        timeval rcv_to{};
        rcv_to.tv_sec = timeout_seconds_;
        rcv_to.tv_usec = 0;
        ::setsockopt(send_fd, SOL_SOCKET, SO_RCVTIMEO, &rcv_to, sizeof(rcv_to));

        // 接收响应数据（最大4096字节）
        BinaryData tcp_response;
        tcp_response.resize(4096);
        ssize_t n = ::recv(send_fd, reinterpret_cast<char*>(tcp_response.data()), static_cast<int>(tcp_response.size()), 0);
        if (n > 0) {
            // 调整响应缓冲区大小到实际接收的数据长度
            tcp_response.resize(static_cast<size_t>(n));
        } else {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // 接收超时
                ::close(send_fd);
                response.result = IControllerModule::OperationResult::TIMEOUT_ERROR;
                response.message = "Receive timeout from target";
                return response;
            }
            // 接收失败或连接关闭，清空响应数据
            tcp_response.clear();
        }
        ::close(send_fd);

        spdlog::info("Received response ({} bytes) from {}:{}: {}", 
                     tcp_response.size(), target_ip, server_port_, ResourceController::binaryToHex(tcp_response));

        // 4) 解析响应数据，提取槽位操作结果和状态信息
        std::string message;
        response.result = parseResponse(tcp_response, slot_numbers, response.slot_results, message);
        response.message = message;

        if (response.result == IControllerModule::OperationResult::SUCCESS || response.result == IControllerModule::OperationResult::PARTIAL_SUCCESS) {
            spdlog::info("Chassis operation completed: {}", message);
        } else {
            spdlog::error("Chassis operation failed: {}", message);
        }

    } catch (const std::runtime_error& e) {
        response.result = IControllerModule::OperationResult::NETWORK_ERROR;
        response.message = std::string("Network error: ") + e.what();
        spdlog::error("Chassis operation network error: {}", response.message);

    } catch (const std::exception& e) {
        response.result = IControllerModule::OperationResult::UNKNOWN_ERROR;
        response.message = std::string("Unknown error: ") + e.what();
        spdlog::error("Chassis operation unknown error: {}", response.message);
    }

    return response;
}

// 构建操作模型结构体，用于封装板卡控制命令
// cmd: 操作命令（"RESET"、"POWON"、"POWOFF"）
// target_ip: 目标机箱IP地址
// slot_numbers: 要操作的槽位号列表（1-12）
// req_id: 请求ID（用于标识请求）
// 返回: 填充好的操作模型结构体
ResourceController::OperationModel ResourceController::buildOperationModel(
    const std::string& cmd,
    const std::string& target_ip,
    const std::vector<int>& slot_numbers,
    uint32_t req_id) const {
    
    ResourceController::OperationModel model;
    
    // 清零结构体，确保所有字段初始化为0
    memset(&model, 0, sizeof(ResourceController::OperationModel));

    // 填充操作标志、目标IP、命令字符串（使用strncpy防止缓冲区溢出）
    strncpy(model.m_strFlag, operation_flag_.c_str(), sizeof(model.m_strFlag) - 1);
    strncpy(model.m_strIp, target_ip.c_str(), sizeof(model.m_strIp) - 1);
    strncpy(model.m_cmd, cmd.c_str(), sizeof(model.m_cmd) - 1);
    model.m_reqId = req_id;
    
    // 设置槽位数组：m_slot[x]中x=槽位号-1，即0对应1槽，11对应第12槽
    // 根据协议文档：m_slot[x]=1表示要对该槽位进行操作，m_slot[x]=0表示不操作
    for (int slot_num : slot_numbers) {
        if (slot_num >= 1 && slot_num <= 12) {
            // 将槽位号转换为数组索引（slot_num-1），并设置为请求操作状态
            model.m_slot[slot_num - 1] = static_cast<char>(IControllerModule::SlotStatus::REQUEST_OPERATION_OR_FAILED); // 1表示要操作
        } else {
            spdlog::warn("Invalid slot number: {}. Valid range is 1-12", slot_num);
        }
    }
    
    return model;
}

// 解析板卡控制操作的响应数据
// response: 接收到的二进制响应数据
// slot_numbers: 请求中指定的槽位号列表
// slot_results: 输出参数，每个槽位的操作结果
// message: 输出参数，解析后的消息字符串
// 返回: 操作结果（成功/部分成功/失败等）
IControllerModule::OperationResult ResourceController::parseResponse(
    const BinaryData& response,
    const std::vector<int>& slot_numbers,
    std::vector<SlotResult>& slot_results,
    std::string& message) const {
    
    // 清空槽位结果列表，准备填充新的解析结果
    // 注意：slot_results中的槽位号对应slot_numbers中的槽位号，不是数组索引
    slot_results.clear();
    
    if (response.empty()) {
        message = "Empty response received";
        return IControllerModule::OperationResult::INVALID_RESPONSE;
    }
    
    // 如果响应数据大小足够，尝试解析为OperationModel结构体
    if (response.size() >= sizeof(ResourceController::OperationModel)) {
        const ResourceController::OperationModel* response_model = 
            reinterpret_cast<const ResourceController::OperationModel*>(response.data());
        
        // 构建响应消息，包含标志、IP、命令、请求ID等信息
        std::ostringstream msg;
        msg << "Response - Flag: " << std::string(response_model->m_strFlag, sizeof(response_model->m_strFlag))
            << ", IP: " << std::string(response_model->m_strIp, sizeof(response_model->m_strIp))
            << ", CMD: " << std::string(response_model->m_cmd, sizeof(response_model->m_cmd))
            << ", ReqID: " << response_model->m_reqId;
        
        // 解析每个槽位的操作状态
        int success_count = 0;
        int failed_count = 0;
        
        // 只处理请求中指定的槽位（不处理未请求的槽位）
        for (size_t i = 0; i < slot_numbers.size(); ++i) {
            int slot_num = slot_numbers[i];
            if (slot_num >= 1 && slot_num <= 12) {
                // 从响应模型中获取该槽位的状态（数组索引为slot_num-1）
                char slot_status = response_model->m_slot[slot_num - 1];
                SlotResult slot_result;
                slot_result.slot_number = slot_num; // 使用实际的槽位号（1-12）
                
                // 根据状态值判断操作结果
                if (slot_status == static_cast<char>(IControllerModule::SlotStatus::NO_OPERATION_OR_SUCCESS)) {
                    // 0表示成功或未操作（这里认为是成功）
                    slot_result.status = IControllerModule::SlotStatus::NO_OPERATION_OR_SUCCESS;
                    success_count++;
                } else if (slot_status == static_cast<char>(IControllerModule::SlotStatus::REQUEST_OPERATION_OR_FAILED)) {
                    // 1表示失败或请求操作（这里认为是失败）
                    slot_result.status = IControllerModule::SlotStatus::REQUEST_OPERATION_OR_FAILED;
                    failed_count++;
                }
                
                slot_results.push_back(slot_result);
            }
        }
        
        // 在消息中添加处理统计信息
        msg << ", Processed slots: " << slot_numbers.size()
            << ", Success: " << success_count 
            << ", Failed: " << failed_count;
        
        message = msg.str();
        
        // 根据成功/失败数量确定整体操作结果
        if (failed_count == 0 && success_count > 0) {
            // 全部成功
            return IControllerModule::OperationResult::SUCCESS;
        } else if (success_count > 0 && failed_count > 0) {
            // 部分成功
            return IControllerModule::OperationResult::PARTIAL_SUCCESS;
        } else if (failed_count > 0) {
            // 全部失败
            return IControllerModule::OperationResult::INVALID_RESPONSE;
        } else {
            // 没有明确失败的情况认为成功
            return IControllerModule::OperationResult::SUCCESS;
        }
    }
        
    // 响应数据格式不正确，默认认为操作成功（兼容性处理）
    return IControllerModule::OperationResult::SUCCESS;
}

} // namespace controller
} // namespace yw

