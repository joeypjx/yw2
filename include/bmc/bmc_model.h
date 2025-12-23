#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

namespace yw {
namespace bmc {

#pragma pack(push, 1)

// 风扇状态格式（6字节）
struct UdpFanInfo {
    std::uint8_t  fanseq;      // 风扇序号（0-5），无此序号时为0xFF
    std::uint8_t  fanmode;     // 告警及工作模式（高4位：告警类型，低4位：工作模式）
    std::uint32_t fanspeed;    // 风扇转速（占空比）
};

// 传感器状态格式（12字节）
struct UdpSensorInfo {
    std::uint8_t  sensorseq;      // 传感器序号（0-7），无此序号时为0xFF
    std::uint8_t  sensortype;     // 传感器类型
    std::uint8_t  sensorname[6];  // 传感器名称
    std::uint8_t  sensorvalue_L;  // 数值低字节（小数部分）
    std::uint8_t  sensorvalue_H;  // 数值高字节（整数部分）
    std::uint8_t  sensoralmtype;  // 告警类型
    std::uint8_t  sensorresv;     // 预留
};

// 电源槽位状态（128字节，对应 ipmc_control.hpp 中的 UdpDyBoardInfo）
struct UdpPowerBoardInfo {
    std::uint8_t  ipmbaddr;       // 槽位地址，槽位号（1字节）
    std::uint16_t moduletype;     // 模块设备号（2字节）
    std::uint16_t bmccompany;     // 厂商编号（2字节）
    std::uint8_t  bmcversion[8];  // BMC软件版本（8字节）
    std::uint8_t  snnum[8];       // 模块板卡序列号（8字节）
    std::uint8_t  protime[8];     // 生产日期（8字节，年月日各2位）
    std::uint8_t  status;         // 板卡健康状态，保留（1字节）
    std::uint8_t  sensornum;      // 传感器数量（1字节，5-8）
    UdpSensorInfo sensor[8];      // 传感器状态（12*8 = 96字节）
    std::uint8_t  resv[1];        // 保留（1字节）
};

// 负载槽位状态（130字节，对应 ipmc_control.hpp 中的 UdpBoardInfo）
struct UdpSlotBoardInfo {
    std::uint8_t  ipmbaddr;       // 槽位地址，槽位号（1字节）
    std::uint16_t moduletype;     // 模块设备号（2字节）
    std::uint8_t  prst;           // 在位信息（1字节，0：不在位，1：在位）
    std::uint16_t bmccompany;     // 厂商编号（2字节）
    std::uint8_t  bmcversion[8];  // BMC软件版本（8字节）
    std::uint8_t  snnum[8];       // 模块板卡序列号（8字节）
    std::uint8_t  protime[8];     // 生产日期（8字节，年月日各2位）
    std::uint8_t  status;         // 板卡健康状态，保留（1字节）
    std::uint8_t  sensornum;      // 传感器数量（1字节，5-8）
    UdpSensorInfo sensor[8];      // 传感器状态（12*8 = 96字节）
    std::uint8_t  resv[2];        // 备用（2字节）
};

// UDP 报文结构（1612字节，对应 ipmc_control.hpp 中的 UdpInfo）
struct UdpInfo {
    std::uint16_t head;           // 报文头（2字节，0x5AA5）
    std::uint16_t msglenth;       // 报文长度（2字节，包含报文头尾）
    std::uint16_t seqnum;         // 编号（2字节，1-65535循环递增）
    std::uint16_t msgtype;        // 报文标识（2字节，0x0002）
    std::uint32_t timestamp;      // 时间戳（4字节）
    std::uint16_t moduletype;     // 模块设备号（2字节）
    std::uint8_t  recv[2];        // 备用（2字节）
    std::uint8_t  boxname;        // 机箱型号（1字节，固定1）
    std::uint8_t  boxid;          // 机箱号（1字节）
    UdpFanInfo    fan[6];         // 风扇1-6状态（6*6 = 36字节，风扇序号0-5，无此风扇序号为0xFF）
    // 电源模块1-2（2*128 = 256字节）
    UdpPowerBoardInfo dyboard[2]; // 电源1/2（2个，每个128字节）
    // 负载槽1,2,3,4,6,7,9,10,11,12（10*130 = 1300字节）
    // 注意：协议中负载槽顺序是：槽1、槽2、槽3、槽4、槽6、槽7、槽9、槽10、槽11、槽12
    UdpSlotBoardInfo board[10];   // 负载1-4 6-7 9-12（10个，每个130字节）
    std::uint16_t tail;            // 报文尾（2字节，0xA55A）
};

#pragma pack(pop)

static_assert(sizeof(UdpFanInfo) == 6, "UdpFanInfo packed size mismatch");
static_assert(sizeof(UdpSensorInfo) == 12, "UdpSensorInfo packed size mismatch");
static_assert(sizeof(UdpPowerBoardInfo) == 128, "UdpPowerBoardInfo packed size mismatch");
static_assert(sizeof(UdpSlotBoardInfo) == 130, "UdpSlotBoardInfo packed size mismatch");
static_assert(sizeof(UdpInfo) == 1612, "UdpInfo packed size mismatch");

struct BMCSensorRow {
    std::int64_t   timestamp = 0;
    std::string    host_ip;
    std::uint16_t  sensorseq = 0;
    std::uint16_t  sensortype = 0;
    std::string    sensorname;
    std::uint16_t  sensorvalue_L = 0; // decimal part
    std::uint16_t  sensorvalue_H = 0; // integer part
    std::double_t  sensor_value = 0; // integer part + decimal part *0.01
    std::uint16_t  sensoralmtype = 0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BMCSensorRow,
    timestamp,
    host_ip,
    sensorseq,
    sensortype,
    sensorname,
    sensorvalue_L,
    sensorvalue_H,
    sensor_value,
    sensoralmtype
)

} // namespace bmc
} // namespace yw


