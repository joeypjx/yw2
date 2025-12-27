// ============================================================================
// 文件功能描述：
// IP地址工具类（IPAddressUtils）的实现文件，提供板卡IP地址的计算和解析功能。
// 主要功能包括：
// 1. IP地址计算：根据机箱号（box_id）和槽位号（slot_id）计算板卡的IP地址
// 2. IP地址解析：从IP地址反向解析出机箱号和槽位号
// 3. IP地址验证：验证IP地址是否为有效的板卡IP地址（格式：192.168.x.y）
// 4. 映射表管理：维护槽位号到host_id的映射关系，支持槽位1-14的完整映射
// 5. 网络ID计算：根据槽位号范围计算network_id（槽位1-7使用偶数，8-14使用奇数）
// 6. 歧义处理：处理某些host_id对应多个slot_id的情况，结合network_id的奇偶性判断
// ============================================================================

#include "utils/ip_address_utils.h"
#include <sstream>
#include <regex>
#include <stdexcept>
#include <optional>

namespace yw {
namespace utils {

// 根据机箱号和槽位号计算板卡IP地址
// 规则：IP地址格式为 192.168.{network_id}.{host_id}
// - network_id: 槽位1-7使用 box_id*2，槽位8-14使用 box_id*2+1
// - host_id: 根据槽位号映射（1->5, 2->37, 3->69, 4->101, 5->133, 6->170, 7->180, 8->5, 9->37, 10->69, 11->101, 12->133, 13->181, 14->182）
std::string IPAddressUtils::calculateHostIP(int box_id, int slot_id) {
    // 验证参数范围
    if (box_id < 1 || box_id > 9) {
        return "";
    }
    
    if (slot_id < 1 || slot_id > 14) {
        return "";
    }
    
    // 获取 host_id
    int host_id = getHostIdBySlotId(slot_id);
    if (host_id == -1) {
        return "";
    }
    
    // 计算 network_id
    int network_id;
    if (slot_id >= 1 && slot_id <= 7) {
        network_id = box_id * 2;
    } else if (slot_id >= 8 && slot_id <= 12) {
        network_id = box_id * 2 + 1;
    } else if (slot_id == 13 || slot_id == 14) {
        // 槽位13和14使用 box_id * 2 + 1
        network_id = box_id * 2 + 1;
    } else {
        return "";
    }
    
    // 构建IP地址
    return "192.168." + std::to_string(network_id) + "." + std::to_string(host_id);
}

// 从IP地址解析出机箱号和槽位号
// 注意：某些host_id对应多个slot_id（如host_id=5对应slot_id=1或8），需要结合network_id的奇偶性判断
std::optional<BoxSlotInfo> IPAddressUtils::parseHostIP(const std::string& host_ip) {
    if (!isValidBoardIP(host_ip)) {
        return std::nullopt;
    }
    
    int network_id, host_id;
    if (!parseIPAddress(host_ip, network_id, host_id)) {
        return std::nullopt;
    }
    
    // 根据 host_id 和 network_id 确定 slot_id
    // 注意：某些 host_id 对应多个 slot_id，需要结合 network_id 判断
    int slot_id = -1;
    
    // 根据 network_id 的奇偶性判断 slot_id 范围
    bool is_even_network = (network_id % 2 == 0);
    
    if (is_even_network) {
        // network_id 为偶数，slot_id 在 1-7 范围内
        switch (host_id) {
            case 5:   slot_id = 1; break;
            case 37:  slot_id = 2; break;
            case 69:  slot_id = 3; break;
            case 101: slot_id = 4; break;
            case 133: slot_id = 5; break;
            case 170: slot_id = 6; break;
            case 180: slot_id = 7; break;
            default:  return std::nullopt;
        }
    } else {
        // network_id 为奇数，slot_id 在 8-14 范围内
        switch (host_id) {
            case 5:   slot_id = 8; break;
            case 37:  slot_id = 9; break;
            case 69:  slot_id = 10; break;
            case 101: slot_id = 11; break;
            case 133: slot_id = 12; break;
            case 181: slot_id = 13; break;
            case 182: slot_id = 14; break;
            default:  return std::nullopt;
        }
    }
    
    if (slot_id == -1) {
        return std::nullopt;
    }
    
    // 根据 network_id 和 slot_id 计算 box_id
    int box_id;
    if (slot_id >= 1 && slot_id <= 7) {
        // network_id = box_id * 2
        box_id = network_id / 2;
    } else if (slot_id >= 8 && slot_id <= 14) {
        // network_id = box_id * 2 + 1
        box_id = (network_id - 1) / 2;
    } else {
        return std::nullopt;
    }
    
    // 验证 box_id 范围
    if (box_id < 1 || box_id > 9) {
        return std::nullopt;
    }
    
    return BoxSlotInfo(box_id, slot_id);
}

// 验证IP地址是否为有效的板卡IP地址
// 有效格式：192.168.x.y，其中x范围2-19，y范围5-182
bool IPAddressUtils::isValidBoardIP(const std::string& host_ip) {
    // 检查格式是否为 192.168.x.y
    std::regex ip_pattern(R"(^192\.168\.(\d{1,3})\.(\d{1,3})$)");
    std::smatch matches;
    
    if (!std::regex_match(host_ip, matches, ip_pattern)) {
        return false;
    }
    
    // 验证第三段和第四段的范围
    try {
        int third = std::stoi(matches[1].str());
        int fourth = std::stoi(matches[2].str());
        
        // 第三段范围：2-19 (box_id 1-9, network_id = box_id*2 或 box_id*2+1)
        if (third < 2 || third > 19) {
            return false;
        }
        
        // 第四段范围：5-182 (根据 host_id 映射表)
        if (fourth < 5 || fourth > 182) {
            return false;
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

// 根据槽位号获取对应的host_id
// 注意：槽位1-7和8-12的host_id有重复，需要结合network_id区分
int IPAddressUtils::getHostIdBySlotId(int slot_id) {
    // slot_id 到 host_id 的映射表（基于 bmc_repository.cpp 中的逻辑）
    switch (slot_id) {
        case 1:  return 5;
        case 2:  return 37;
        case 3:  return 69;
        case 4:  return 101;
        case 5:  return 133;
        case 6:  return 170;
        case 7:  return 180;
        case 8:  return 5;
        case 9:  return 37;
        case 10: return 69;
        case 11: return 101;
        case 12: return 133;
        case 13: return 181;
        case 14: return 182;
        default: return -1;
    }
}

int IPAddressUtils::getSlotIdByHostId(int host_id) {
    // host_id 到 slot_id 的映射（注意：某些 host_id 对应多个 slot_id）
    // 这个函数返回第一个可能的 slot_id，实际使用时需要结合 network_id 判断
    switch (host_id) {
        case 5:   return 1;   // slot_id 1 或 8
        case 37:  return 2;   // slot_id 2 或 9
        case 69:  return 3;   // slot_id 3 或 10
        case 101: return 4;   // slot_id 4 或 11
        case 133: return 5;   // slot_id 5 或 12
        case 170: return 6;   // slot_id 6
        case 180: return 7;   // slot_id 7
        case 181: return 13;  // slot_id 13
        case 182: return 14; // slot_id 14
        default:  return -1;
    }
}

// 解析IP地址字符串，提取network_id和host_id
// 格式：192.168.{network_id}.{host_id}
bool IPAddressUtils::parseIPAddress(const std::string& host_ip, int& network_id, int& host_id) {
    std::regex ip_pattern(R"(^192\.168\.(\d{1,3})\.(\d{1,3})$)");
    std::smatch matches;
    
    if (!std::regex_match(host_ip, matches, ip_pattern)) {
        return false;
    }
    
    try {
        network_id = std::stoi(matches[1].str());
        host_id = std::stoi(matches[2].str());
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace utils
} // namespace yw

