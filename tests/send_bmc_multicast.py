#!/usr/bin/env python3
"""
BMC 组播报文发送脚本
发送 UdpInfo 格式的 UDP 组播报文
格式参见 include/bmc/bmc_model.h
"""

import struct
import socket
import time
import sys
import random
from typing import Optional

# 默认配置（从 config.json 读取）
DEFAULT_MCAST_GROUP = "224.100.200.15"
DEFAULT_MCAST_PORT = 5715
DEFAULT_INTERVAL = 5  # 秒

# 协议常量
HEAD_MAGIC = 0x5AA5
TAIL_MAGIC = 0xA55A
MSG_TYPE = 0x0002
UDP_INFO_SIZE = 1612  # 总长度


def create_udp_fan_info(fanseq: int, fanmode: int = 0, fanspeed: int = 50) -> bytes:
    """
    创建 UdpFanInfo 结构（6字节）
    fanseq: 风扇序号（0-5），无此风扇时为 0xFF
    fanmode: 告警及工作模式（高4位：告警类型，低4位：工作模式）
    fanspeed: 风扇转速（占空比）
    """
    if fanseq == 0xFF:
        # 风扇不存在，使用默认值
        return struct.pack('<BBL', 0xFF, 0, 0)
    return struct.pack('<BBL', fanseq & 0xFF, fanmode & 0xFF, fanspeed & 0xFFFFFFFF)


def create_udp_sensor_info(
    sensorseq: int,
    sensortype: int = 0,
    sensorname: str = "",
    sensorvalue_H: int = 0,
    sensorvalue_L: int = 0,
    sensoralmtype: int = 0
) -> bytes:
    """
    创建 UdpSensorInfo 结构（12字节）
    sensorseq: 传感器序号（0-7），无此传感器时为 0xFF
    sensortype: 传感器类型
    sensorname: 传感器名称（最多6字节）
    sensorvalue_H: 数值高字节（整数部分）
    sensorvalue_L: 数值低字节（小数部分）
    sensoralmtype: 告警类型
    """
    # 确保传感器名称不超过6字节
    name_bytes = sensorname.encode('ascii', errors='ignore')[:6].ljust(6, b'\x00')
    if sensorseq == 0xFF:
        # 传感器不存在
        return struct.pack('<BB6sBBBB', 0xFF, 0, b'\x00' * 6, 0, 0, 0, 0)
    return struct.pack('<BB6sBBBB', sensorseq & 0xFF, sensortype & 0xFF, name_bytes,
                       sensorvalue_L & 0xFF, sensorvalue_H & 0xFF, sensoralmtype & 0xFF, 0)


def create_udp_power_board_info(
    ipmbaddr: int,
    moduletype: int = 0,
    bmccompany: int = 0,
    bmcversion: str = "1.0.0",
    snnum: str = "00000000",
    protime: str = "240101",  # YYMMDD
    status: int = 0,
    sensornum: int = 5,
    sensors: Optional[list] = None
) -> bytes:
    """
    创建 UdpPowerBoardInfo 结构（128字节）
    """
    # 确保字符串长度正确
    version_bytes = bmcversion.encode('ascii', errors='ignore')[:8].ljust(8, b'\x00')
    sn_bytes = snnum.encode('ascii', errors='ignore')[:8].ljust(8, b'\x00')
    time_bytes = protime.encode('ascii', errors='ignore')[:8].ljust(8, b'\x00')
    
    # 创建传感器数组（8个）
    sensor_data = b''
    if sensors:
        for i in range(8):
            if i < len(sensors):
                sensor_data += sensors[i]
            else:
                sensor_data += create_udp_sensor_info(0xFF)  # 空传感器
    else:
        # 创建默认传感器
        for i in range(8):
            if i < sensornum:
                sensor_data += create_udp_sensor_info(
                    i, 0, f"SENS{i}", random.randint(20, 80), 0, 0
                )
            else:
                sensor_data += create_udp_sensor_info(0xFF)
    
    # 打包：1 + 2 + 2 + 8 + 8 + 8 + 1 + 1 + 96 + 1 = 128
    return struct.pack('<BHH8s8s8sBB', 
                      ipmbaddr & 0xFF,
                      moduletype & 0xFFFF,
                      bmccompany & 0xFFFF,
                      version_bytes,
                      sn_bytes,
                      time_bytes,
                      status & 0xFF,
                      sensornum & 0xFF) + sensor_data + struct.pack('<B', 0)


def create_udp_slot_board_info(
    ipmbaddr: int,
    moduletype: int = 0,
    prst: int = 1,  # 1: 在位，0: 不在位
    bmccompany: int = 0,
    bmcversion: str = "1.0.0",
    snnum: str = "00000000",
    protime: str = "240101",  # YYMMDD
    status: int = 0,
    sensornum: int = 5,
    sensors: Optional[list] = None
) -> bytes:
    """
    创建 UdpSlotBoardInfo 结构（130字节）
    """
    # 确保字符串长度正确
    version_bytes = bmcversion.encode('ascii', errors='ignore')[:8].ljust(8, b'\x00')
    sn_bytes = snnum.encode('ascii', errors='ignore')[:8].ljust(8, b'\x00')
    time_bytes = protime.encode('ascii', errors='ignore')[:8].ljust(8, b'\x00')
    
    # 创建传感器数组（8个）
    sensor_data = b''
    if sensors:
        for i in range(8):
            if i < len(sensors):
                sensor_data += sensors[i]
            else:
                sensor_data += create_udp_sensor_info(0xFF)  # 空传感器
    else:
        # 创建默认传感器
        for i in range(8):
            if i < sensornum:
                sensor_data += create_udp_sensor_info(
                    i, 0, f"SENS{i}", random.randint(20, 80), 0, 0
                )
            else:
                sensor_data += create_udp_sensor_info(0xFF)
    
    # 打包：1 + 2 + 1 + 2 + 8 + 8 + 8 + 1 + 1 + 96 + 2 = 130
    return struct.pack('<BHBH8s8s8sBB',
                      ipmbaddr & 0xFF,
                      moduletype & 0xFFFF,
                      prst & 0xFF,
                      bmccompany & 0xFFFF,
                      version_bytes,
                      sn_bytes,
                      time_bytes,
                      status & 0xFF,
                      sensornum & 0xFF) + sensor_data + struct.pack('<H', 0)


def create_udp_info(
    box_id: int = 1,
    seqnum: int = 1,
    moduletype: int = 0,
    fans: Optional[list] = None,
    power_boards: Optional[list] = None,
    slot_boards: Optional[list] = None
) -> bytes:
    """
    创建完整的 UdpInfo 结构（1612字节）
    """
    # 创建默认风扇数据（6个）
    fan_data = b''
    if fans:
        for i in range(6):
            if i < len(fans):
                fan_data += fans[i]
            else:
                fan_data += create_udp_fan_info(0xFF)  # 空风扇
    else:
        # 创建默认风扇（前2个存在）
        for i in range(6):
            if i < 2:
                fan_data += create_udp_fan_info(
                    i, 0, random.randint(30, 80)  # 风扇转速 30-80%
                )
            else:
                fan_data += create_udp_fan_info(0xFF)
    
    # 创建默认电源板数据（2个）
    power_board_data = b''
    if power_boards:
        for board in power_boards:
            power_board_data += board
    else:
        for i in range(2):
            power_board_data += create_udp_power_board_info(
                ipmbaddr=i+1,
                moduletype=0x0100 + i,
                sensornum=5
            )
    
    # 创建默认负载槽板数据（10个，对应槽1,2,3,4,6,7,9,10,11,12）
    slot_board_data = b''
    slot_ids = [1, 2, 3, 4, 6, 7, 9, 10, 11, 12]  # 协议定义的槽位顺序
    if slot_boards:
        for board in slot_boards:
            slot_board_data += board
    else:
        for i, slot_id in enumerate(slot_ids):
            # 随机决定是否在位
            prst = 1 if random.random() > 0.3 else 0
            slot_board_data += create_udp_slot_board_info(
                ipmbaddr=slot_id,
                moduletype=0x0200 + i,
                prst=prst,
                sensornum=5 if prst == 1 else 0
            )
    
    # 时间戳（Unix 时间戳）
    timestamp = int(time.time())
    
    # 打包头部：2 + 2 + 2 + 2 + 4 + 2 + 2 + 1 + 1 = 18
    header = struct.pack('<HHHHIHBBBB',
                        HEAD_MAGIC,      # head (H)
                        UDP_INFO_SIZE,   # msglenth (H)
                        seqnum & 0xFFFF, # seqnum (H)
                        MSG_TYPE,        # msgtype (H)
                        timestamp,       # timestamp (I)
                        moduletype & 0xFFFF,  # moduletype (H)
                        0,               # recv[0] (B)
                        0,               # recv[1] (B)
                        1,               # boxname (B, 固定1)
                        box_id & 0xFF    # boxid (B)
    )
    
    # 打包尾部：2
    tail = struct.pack('<H', TAIL_MAGIC)
    
    # 组合完整报文
    packet = header + fan_data + power_board_data + slot_board_data + tail
    
    # 验证长度
    if len(packet) != UDP_INFO_SIZE:
        raise ValueError(f"Packet size mismatch: expected {UDP_INFO_SIZE}, got {len(packet)}")
    
    return packet


def send_multicast(
    packet: bytes,
    mcast_group: str = DEFAULT_MCAST_GROUP,
    mcast_port: int = DEFAULT_MCAST_PORT,
    ttl: int = 1
) -> bool:
    """
    发送 UDP 组播报文
    """
    try:
        # 创建 UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        
        # 设置 socket 选项
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # 发送到组播地址
        sock.sendto(packet, (mcast_group, mcast_port))
        sock.close()
        
        return True
    except Exception as e:
        print(f"发送组播报文失败: {e}")
        return False


def main():
    """主函数"""
    # 解析命令行参数
    box_id = 1
    mcast_group = DEFAULT_MCAST_GROUP
    mcast_port = DEFAULT_MCAST_PORT
    interval = DEFAULT_INTERVAL
    seqnum = 1
    
    if len(sys.argv) > 1:
        try:
            box_id = int(sys.argv[1])
        except ValueError:
            print(f"错误：box_id 必须是数字，使用默认值 {box_id}")
    
    if len(sys.argv) > 2:
        mcast_group = sys.argv[2]
    
    if len(sys.argv) > 3:
        try:
            mcast_port = int(sys.argv[3])
        except ValueError:
            print(f"错误：端口号必须是数字，使用默认端口 {mcast_port}")
    
    if len(sys.argv) > 4:
        try:
            interval = int(sys.argv[4])
        except ValueError:
            print(f"错误：间隔时间必须是数字，使用默认间隔 {interval} 秒")
    
    print(f"BMC 组播报文发送脚本")
    print(f"组播地址: {mcast_group}:{mcast_port}")
    print(f"机箱号 (box_id): {box_id}")
    print(f"发送间隔: {interval} 秒")
    print(f"按 Ctrl+C 停止\n")
    
    try:
        while True:
            # 创建 UdpInfo 报文
            packet = create_udp_info(
                box_id=box_id,
                seqnum=seqnum,
                moduletype=0x0001
            )
            
            # 发送组播报文
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
            if send_multicast(packet, mcast_group, mcast_port):
                print(f"[{timestamp}] 发送成功 - box_id={box_id}, seqnum={seqnum}, 长度={len(packet)} 字节")
            else:
                print(f"[{timestamp}] 发送失败")
            
            # 递增序列号（1-65535 循环）
            seqnum = (seqnum % 65535) + 1
            
            # 等待指定时间
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print(f"\n\n脚本已停止")


if __name__ == "__main__":
    main()

