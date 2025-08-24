#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

namespace yw {
namespace bmc {

#pragma pack(push, 1)

struct UdpFanInfo {
    std::uint8_t  fanseq;
    std::uint8_t  fanmode;
    std::uint32_t fanspeed;
};

struct UdpSensorInfo {
    std::uint8_t  sensorseq;
    std::uint8_t  sensortype;
    std::uint8_t  sensorname[6];
    std::uint8_t  sensorvalue_L;
    std::uint8_t  sensorvalue_H;
    std::uint8_t  sensoralmtype;
    std::uint8_t  sensorresv;
};

struct UdpBoardInfo {
    std::uint8_t  ipmbaddr;
    std::uint16_t moduletype;
    std::uint16_t bmccompany;
    std::uint8_t  bmcversion[8];
    std::uint8_t  sensornum;
    UdpSensorInfo sensor[5];
    std::uint8_t  resv[2];
};

struct UdpInfo {
    std::uint16_t head;
    std::uint16_t msglenth;
    std::uint16_t seqnum;
    std::uint16_t msgtype;
    std::uint32_t timestamp;
    std::uint8_t  recv[4];
    std::uint8_t  boxname;
    std::uint8_t  boxid;
    UdpFanInfo    fan[2];
    UdpBoardInfo  board[14];
    std::uint16_t tail;
};

#pragma pack(pop)

static_assert(sizeof(UdpFanInfo) == 6, "UdpFanInfo packed size mismatch");
static_assert(sizeof(UdpSensorInfo) == 12, "UdpSensorInfo packed size mismatch");
static_assert(sizeof(UdpBoardInfo) == 76, "UdpBoardInfo packed size mismatch");
static_assert(sizeof(UdpInfo) == 1096, "UdpInfo packed size mismatch");

struct BMCSensorRow {
    std::int64_t   timestamp = 0;
    std::string    host_ip;
    std::uint16_t  sensorseq = 0;
    std::uint16_t  sensortype = 0;
    std::string    sensorname;
    std::uint16_t  sensorvalue_L = 0;
    std::uint16_t  sensorvalue_H = 0;
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
    sensoralmtype
)

} // namespace bmc
} // namespace yw


