#ifndef __IPMC_CONTROL_H__
#define __IPMC_CONTROL_H__

#include <sys/sysinfo.h>
#include <systemd/sd-journal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <time.h>

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include <gpiod.hpp>
#include <phosphor-logging/log.hpp>

#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/message.hpp>
#include <systemd/sd-bus.h>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <variant>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <glob.h>
 namespace ipmc_control
{  
    static std:: string group_ip = "224.100.200.15";//组播地址
    static uint16_t group_port = 5715; // 组播端口
    
    static bool isEnable = true; // 组播开关
    static uint8_t time_interval = 10;//时间间隔
using DbusVariant = std::variant<std::string, bool, uint8_t, uint16_t, int16_t,
                                 uint32_t, int32_t, uint64_t, int64_t, double,std::vector<uint8_t>>;//
using BasicVariantType =
    std::variant<std::vector<std::string>, std::vector<uint64_t>,
                 std::vector<uint8_t>, std::string, int64_t, uint64_t, double,
                 int32_t, uint32_t, int16_t, uint16_t, uint8_t, bool>;   //                              
using PropertyMapType =
    boost::container::flat_map<std::string, BasicVariantType>;//
    //0x61结构体
typedef struct __attribute__((packed))
{
    uint8_t sensor_id; // 传感器序号
    uint8_t sensor_type; // 传感器类型
    char sensor_name[6]; // 传感器名称
    uint16_t high_critical_alarm; // 高临界告警阈值
    uint16_t high_fault_alarm; // 高故障告警阈值
    uint16_t low_critical_alarm; // 低临界告警阈值
    uint16_t low_fault_alarm; // 低故障告警阈值
}SensorInfo;
/*和风扇小板通信
1：风扇序号
2：告警类型及工作模式，各占 4bit。高 4 位
表示告警类型，低 4 位表示工作模式。0,zidong ;1,shoudong
3：转速。高 1 位表示转速单位，后 7 位表示转速数值。
转速单位为 0 时表示档位，档位定义：低速:1，中速:2，高速:3；
转速单位为1  时表示占空比，此时转速数值为转速占空比，取值 1-100。
*/
typedef struct __attribute__((packed))
{
  uint8_t fanseq;//序号
  uint8_t fanmode;//告警及工作模式，高4位告警类型，低4位工作模式
  uint32_t fanspeed;//转速
}UdpFanInfo;

typedef struct __attribute__((packed))
{
  uint8_t sensorseq;//传感器序号
  uint8_t sensortype;//传感器类型
  uint8_t sensorname[6];//传感器名称
  uint8_t sensorvalue_L;//数值
  uint8_t sensorvalue_H;//数值
  uint8_t sensoralmtype;//告警类型
  uint8_t sensorresv;//预留

}UdpSensorInfo;

typedef struct __attribute__((packed))
{
uint8_t ipmbaddr;//槽位地址，槽位号
uint16_t moduletype;//模块设备号
uint16_t bmccompany;//厂商编号
uint8_t bmcversion[8];//BMC软件版本
uint8_t snnum[8];//模块板卡序列号
uint8_t protime[8];// 生产日期 8字节,年月日各两位
uint8_t status;//板卡健康状态，保留
uint8_t sensornum;//5-8
UdpSensorInfo sensor[8];
uint8_t resv[1];//0
}UdpDyBoardInfo;

typedef struct __attribute__((packed))
{
uint8_t ipmbaddr;//槽位地址，槽位号
uint16_t moduletype;//模块设备号
uint8_t prst;//在位信息，0不在，1在位
uint16_t bmccompany;//厂商编号
uint8_t bmcversion[8];//BMC软件版本
uint8_t snnum[8];//模块板卡序列号
uint8_t protime[8];// 生产日期 8字节,年月日各两位
uint8_t status;//板卡健康状态，保留
uint8_t sensornum;//5-8
UdpSensorInfo sensor[8];
uint8_t resv[2];//0
}UdpBoardInfo;


typedef struct __attribute__((packed))
{
uint16_t head;//0x5AA5
uint16_t msglenth;//报文长度
uint16_t seqnum;//1-65535循环
uint16_t msgtype;//表征报文种类，信号处理取0x0002
uint32_t timestamp;//时间戳
uint16_t moduletype;//模块设备号
uint8_t recv[2];
uint8_t boxname;//固定1
uint8_t boxid;
UdpFanInfo fan[6];//风扇序号0-5，无此风扇序号为0xFF
UdpDyBoardInfo dyboard[2];//电源1/2，
UdpBoardInfo board[10];//负载1-4 6-7 9-12
uint16_t tail;//0x5AA5
}UdpInfo;


typedef struct __attribute__((packed)) 
{
    uint8_t sensornum;
    uint8_t sensor_high;    
    uint8_t sensor_low;     // 数值
    uint8_t alarmtype; // 告警类型
} SensorValueInfo;

typedef struct __attribute__((packed))
{
    uint8_t header[4];
    uint8_t sensor_id; // 传感器序号
    uint8_t sensormode; // 高4位告警类型、低4位工作模式
    uint8_t sensordata; // 高1位表示单位 为1 pwm值 或 为0 档位
}FanInfo;
//组播配置
//name:GroupSender
//function:组播配置
//输入：
//输出：
//备注：
// 
class GroupSender
    {
    public:
        GroupSender(int domain, int type, int protocol)
            : _sock(socket(domain, type, protocol))
        {
            if (_sock < 0)
            {
                throw std::runtime_error("socket error");
            }
        }
        GroupSender(const GroupSender &) = delete;
        GroupSender &operator=(const GroupSender &) = delete;
        ~GroupSender()
        {
            if (_sock >= 0)
                close(_sock);
        }

        int getsockfd() const
        {
            return _sock;
        }
        std::string getbindip() const
        {
            return _bindip;
        }
        int updateSrcip(sockaddr_in &addr, int ethid)
        {
           // std::cerr << "mutiudp updateSrcip/n";
            std::string interfaceName = "eth" + std::to_string(ethid);
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);
            if (ioctl(_sock, SIOCGIFADDR, &ifr) < 0)
            {
                std::cerr << "ioctl error/n";
                return -2;
            }
            // 设置默认本地ip地址 //排除其他网口造成的干扰
            struct in_addr localInterface;
            _bindip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
            localInterface.s_addr = inet_addr(_bindip.c_str());

            if (setsockopt(_sock, IPPROTO_IP, IP_MULTICAST_IF,
                           (char *)&localInterface, sizeof(localInterface)) < 0)
            {
                std::cerr << "Error setting multicast interface" << std::endl;
                return -1;
            }

            return 0;
        }

    private:
        int _sock;           // 套接字
        std::string _bindip; // 本地ip地址
    };
    
    
//所有传感器
class SensorCombinedData {
public:
    std::vector<SensorInfo> basic_info;
    std::vector<SensorValueInfo> value_info; // 存储多个传感器的数值信息

    void updateBasicInfo(const std::vector<SensorInfo>& info) {
        basic_info = info;
    }

    void updateValueInfo(const std::vector<SensorValueInfo>& values) {
        value_info = values;
    }
    std::vector<uint8_t> getAlarmdata()
    {
        std::vector<uint8_t>Alarmdata;
        std::unordered_map<uint8_t, SensorInfo> basicInfoMap;
        if(basic_info.size() != (value_info.size()))
            {
                std::cout<<"sensor count not match \n";
                return Alarmdata;
            }
        for (const auto& basic : basic_info) {
            basicInfoMap[basic.sensor_id] = basic;
        }
        for(const auto& data: value_info)
        {
            if(data.alarmtype != 0)
            {
                if (basicInfoMap.find(data.sensornum) != basicInfoMap.end()) 
                {
                    const auto& basic = basicInfoMap[data.sensornum];
                    // 打包传感器信息
                    packSensorInfo(basic.sensor_type, data, Alarmdata);
                }
                else
                {
                     std::cout<<"basicinfo find err \n";
                }
            }
            else
            {
                std::cout<<"sensor "<<data.sensornum << "is OK \n";
            }

        }
        return Alarmdata;
    };
    private:
    void packSensorInfo(uint8_t sensortype, const SensorValueInfo& value, std::vector<uint8_t>& buffer) const 
    {
        buffer.reserve(buffer.size() + sizeof(SensorValueInfo) + 1);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(SensorValueInfo));
        buffer.emplace_back(sensortype);
    }
};

//板卡数据
class CardData 
{
public:
    uint8_t cardnum;
    std::unique_ptr<SensorCombinedData> sensor_data;
    
    CardData() : cardnum(7), sensor_data(std::make_unique<SensorCombinedData>()) {}
    CardData(uint8_t cardnum) : cardnum(cardnum), sensor_data(std::make_unique<SensorCombinedData>()) {}

    void updateBasicInfo(const std::vector<SensorInfo>& info) {
        sensor_data->updateBasicInfo(info);
    }

    void updateValueInfo(const std::vector<SensorValueInfo>& values) {
        sensor_data->updateValueInfo(values);
    }
    std::vector<uint8_t> getAlarmSensors() const {
        return sensor_data->getAlarmdata();
        }

};

std::vector<CardData> cardinfo(14);

void initializeCards() 
{
  // 初始化每个板卡的 cardnum，其他字段保持为空值
    for (size_t i = 0; i <= 13; ++i) {  //设置为14块板卡
        // 直接设置 cardnum
        cardinfo[i].cardnum = static_cast<uint8_t>(i+1);    //板卡号
        // 初始化传感器数据为默认值
        cardinfo[i].sensor_data->basic_info.clear();
        cardinfo[i].sensor_data->value_info.clear();
    }
}

    template<typename T>
    std::vector<T> getSensorData(const std::vector<uint8_t> responseData)
    {
    std::vector<T> dataItems;
    if(responseData.size() <=7)
    {
        std::cerr << "sensordata size err " << responseData.size() << "\n";
        return dataItems;
    }   
        
    std::vector<uint8_t> sensordata(responseData.begin()+4,responseData.end()); //跳过头信息
    size_t numItems = sensordata.size() / (sizeof(T));
    // std::cerr<<"numItems"<< numItems<<"\n";
    for(size_t i =0; i<numItems;i++)
    {
        T item;
        if ((i + 1) * (sizeof(T)) <= sensordata.size()) 
        {
            memcpy(&item, &sensordata[i * sizeof(T)], sizeof(T));
            dataItems.emplace_back(item);
        } 
        else 
        {
            // 处理不完整的数据
        std::cerr << "Warning: Incomplete data at index " << i << "\n";
        break;
        }
    }
        return dataItems;
    }
    
    void updateCardInfo(uint8_t cardindex,std::vector<uint8_t>responseData,uint8_t datatype)
    {
        if(cardindex >= 14)
            return;
        switch (datatype)
        {
            case 0:
            {
                std::vector<SensorValueInfo> valueInfos = getSensorData<SensorValueInfo>(responseData);
                if (!valueInfos.empty()) 
                {
                    // 更新数据信息
                    cardinfo[cardindex].updateValueInfo(valueInfos);
                }
                // std::cout<<"valueInfos.size() = " <<valueInfos.size()<<"\n";
                return;
            } 
            break;
            case 1:
            {
                std::vector<SensorInfo> basicInfos = getSensorData<SensorInfo>(responseData);
                if (!basicInfos.empty()) {
                    // 更新基本信息
                    cardinfo[cardindex].updateBasicInfo(basicInfos);
                }
                // std::cout<<"basicInfos.size() = " <<basicInfos.size()<<"\n";
                return;
            } 
            break;
            default:
                break;
        }
    }
} 
#endif 

