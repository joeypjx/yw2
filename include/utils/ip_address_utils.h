#pragma once

#include <string>
#include <optional>

namespace yw {
namespace utils {

/**
 * @brief 机箱槽位信息结构体
 */
struct BoxSlotInfo {
    int box_id;   // 机箱号
    int slot_id;  // 槽位号
    
    BoxSlotInfo(int box, int slot) : box_id(box), slot_id(slot) {}
};

/**
 * @brief IP地址与机箱槽位转换工具类
 * 
 * 提供板卡IP地址与机箱号、槽位号之间的相互转换功能
 * 
 * IP地址格式：192.168.{network_id}.{host_id}
 * - network_id: 根据 box_id 和 slot_id 计算
 * - host_id: 根据 slot_id 映射
 * 
 * 映射规则：
 * - slot_id 1-7: network_id = box_id * 2, host_id 有固定映射
 * - slot_id 8-12: network_id = box_id * 2 + 1, host_id 有固定映射
 */
class IPAddressUtils {
public:
    /**
     * @brief 根据机箱号和槽位号计算IP地址
     * @param box_id 机箱号 (1-9)
     * @param slot_id 槽位号 (1-12, 13-14为特殊槽位)
     * @return IP地址字符串，如果参数无效返回空字符串
     */
    static std::string calculateHostIP(int box_id, int slot_id);
    
    /**
     * @brief 根据IP地址解析机箱号和槽位号
     * @param host_ip IP地址字符串，格式为 "192.168.x.y"
     * @return 包含 box_id 和 slot_id 的结构体，如果解析失败返回 std::nullopt
     */
    static std::optional<BoxSlotInfo> parseHostIP(const std::string& host_ip);
    
    /**
     * @brief 检查IP地址是否为有效的板卡IP地址格式
     * @param host_ip IP地址字符串
     * @return 是否为有效格式
     */
    static bool isValidBoardIP(const std::string& host_ip);

private:
    /**
     * @brief 根据槽位号获取 host_id
     * @param slot_id 槽位号
     * @return host_id，如果槽位号无效返回 -1
     */
    static int getHostIdBySlotId(int slot_id);
    
    /**
     * @brief 根据 host_id 获取槽位号（需要结合 network_id 判断）
     * @param host_id IP地址的第四段
     * @return 槽位号，如果 host_id 无效返回 -1
     * @note 某些 host_id 对应多个 slot_id，实际使用时需要结合 network_id 判断
     */
    static int getSlotIdByHostId(int host_id);
    
    /**
     * @brief 解析IP地址字符串
     * @param host_ip IP地址字符串
     * @param network_id 输出的第三段网络ID
     * @param host_id 输出的第四段主机ID
     * @return 是否解析成功
     */
    static bool parseIPAddress(const std::string& host_ip, int& network_id, int& host_id);
};

} // namespace utils
} // namespace yw

