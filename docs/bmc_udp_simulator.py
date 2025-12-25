#!/usr/bin/env python3
"""
BMC UDP 数据模拟器
模拟发送 UdpInfo 结构体的 UDP 组播数据包
组播地址: 224.100.200.15
组播端口: 5715
"""

import struct
import socket
import time
import argparse
from datetime import datetime


class BMCUdpSimulator:
    """BMC UDP 数据包模拟器"""
    
    # 组播地址和端口
    MULTICAST_ADDR = "224.100.200.15"
    MULTICAST_PORT = 5715
    
    # 报文常量
    HEAD = 0x5AA5
    TAIL = 0xA55A
    MSG_TYPE = 0x0002
    MSG_LENGTH = 1612  # 总报文长度
    
    def __init__(self, box_id=1, module_type=0x0001, interval=1.0):
        """
        初始化模拟器
        
        Args:
            box_id: 机箱号 (1-255)
            module_type: 模块设备号
            interval: 发送间隔（秒）
        """
        self.box_id = box_id
        self.module_type = module_type
        self.interval = interval
        self.seqnum = 1
        
        # 创建 UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
        
    def pack_fan_info(self, fanseq, fanmode=0, fanspeed=50):
        """
        打包风扇信息 (6字节)
        
        Args:
            fanseq: 风扇序号 (0-5)，无此序号时为 0xFF
            fanmode: 告警及工作模式
            fanspeed: 风扇转速（占空比）
        """
        return struct.pack('<BBL', fanseq, fanmode, fanspeed)
    
    def pack_sensor_info(self, sensorseq, sensortype=0, sensorname=b'      ', 
                         sensorvalue_L=0, sensorvalue_H=25, sensoralmtype=0):
        """
        打包传感器信息 (12字节)
        
        Args:
            sensorseq: 传感器序号 (0-7)，无此序号时为 0xFF
            sensortype: 传感器类型
            sensorname: 传感器名称 (6字节)
            sensorvalue_L: 数值低字节（小数部分）
            sensorvalue_H: 数值高字节（整数部分）
            sensoralmtype: 告警类型
        """
        if isinstance(sensorname, str):
            sensorname = sensorname.encode('ascii')[:6].ljust(6, b' ')
        return struct.pack('<BB6sBBBB', sensorseq, sensortype, sensorname, 
                          sensorvalue_L, sensorvalue_H, sensoralmtype, 0)
    
    def pack_power_board_info(self, ipmbaddr, sensornum=5):
        """
        打包电源槽位信息 (128字节)
        
        Args:
            ipmbaddr: 槽位地址，槽位号
            sensornum: 传感器数量 (5-8)
        """
        data = bytearray(128)
        data[0] = ipmbaddr  # ipmbaddr
        struct.pack_into('<H', data, 1, self.module_type)  # moduletype
        struct.pack_into('<H', data, 3, 0x0001)  # bmccompany
        # bmcversion (8字节)
        bmcversion = b'V1.0.0  '
        data[5:13] = bmcversion[:8]
        # snnum (8字节)
        snnum = f'SN{ipmbaddr:06d}'.encode('ascii')[:8].ljust(8, b' ')
        data[13:21] = snnum
        # protime (8字节，年月日各2位)
        now = datetime.now()
        protime = struct.pack('>HHHH', now.year % 100, now.month, now.day, 0)
        data[21:25] = protime[:4]
        data[25:29] = b'\x00' * 4
        data[29] = 0  # status
        data[30] = sensornum  # sensornum
        
        # 传感器信息 (8个，每个12字节)
        offset = 31
        for i in range(8):
            if i < sensornum:
                sensor = self.pack_sensor_info(
                    sensorseq=i,
                    sensortype=1,
                    sensorname=f'SENS{i}',
                    sensorvalue_H=20 + i * 2,
                    sensorvalue_L=50
                )
            else:
                sensor = self.pack_sensor_info(sensorseq=0xFF)
            data[offset:offset+12] = sensor
            offset += 12
        
        data[127] = 0  # resv
        return bytes(data)
    
    def pack_slot_board_info(self, ipmbaddr, prst=1, sensornum=5):
        """
        打包负载槽位信息 (130字节)
        
        Args:
            ipmbaddr: 槽位地址，槽位号
            prst: 在位信息 (0：不在位，1：在位)
            sensornum: 传感器数量 (5-8)
        """
        data = bytearray(130)
        data[0] = ipmbaddr  # ipmbaddr
        struct.pack_into('<H', data, 1, self.module_type)  # moduletype
        data[3] = prst  # prst
        struct.pack_into('<H', data, 4, 0x0001)  # bmccompany
        # bmcversion (8字节)
        bmcversion = b'V1.0.0  '
        data[6:14] = bmcversion[:8]
        # snnum (8字节)
        snnum = f'SN{ipmbaddr:06d}'.encode('ascii')[:8].ljust(8, b' ')
        data[14:22] = snnum
        # protime (8字节，年月日各2位)
        now = datetime.now()
        protime = struct.pack('>HHHH', now.year % 100, now.month, now.day, 0)
        data[22:26] = protime[:4]
        data[26:30] = b'\x00' * 4
        data[30] = 0  # status
        data[31] = sensornum  # sensornum
        
        # 传感器信息 (8个，每个12字节)
        offset = 32
        for i in range(8):
            if i < sensornum:
                sensor = self.pack_sensor_info(
                    sensorseq=i,
                    sensortype=1,
                    sensorname=f'SENS{i}',
                    sensorvalue_H=20 + i * 2,
                    sensorvalue_L=50
                )
            else:
                sensor = self.pack_sensor_info(sensorseq=0xFF)
            data[offset:offset+12] = sensor
            offset += 12
        
        data[128:130] = b'\x00\x00'  # resv
        return bytes(data)
    
    def build_udp_packet(self):
        """
        构建完整的 UDP 数据包 (1612字节)
        """
        packet = bytearray(self.MSG_LENGTH)
        offset = 0
        
        # head (2字节)
        struct.pack_into('<H', packet, offset, self.HEAD)
        offset += 2
        
        # msglenth (2字节)
        struct.pack_into('<H', packet, offset, self.MSG_LENGTH)
        offset += 2
        
        # seqnum (2字节)
        struct.pack_into('<H', packet, offset, self.seqnum)
        offset += 2
        
        # msgtype (2字节)
        struct.pack_into('<H', packet, offset, self.MSG_TYPE)
        offset += 2
        
        # timestamp (4字节，Unix 时间戳）
        timestamp = int(time.time())
        struct.pack_into('<I', packet, offset, timestamp)
        offset += 4
        
        # moduletype (2字节)
        struct.pack_into('<H', packet, offset, self.module_type)
        offset += 2
        
        # recv (2字节，备用)
        packet[offset:offset+2] = b'\x00\x00'
        offset += 2
        
        # boxname (1字节，固定1)
        packet[offset] = 1
        offset += 1
        
        # boxid (1字节)
        packet[offset] = self.box_id
        offset += 1
        
        # fan[6] (36字节)
        for i in range(6):
            fan = self.pack_fan_info(
                fanseq=i,
                fanmode=0,
                fanspeed=50 + i * 5
            )
            packet[offset:offset+6] = fan
            offset += 6
        
        # dyboard[2] (256字节，电源模块1-2)
        for i in range(2):
            power_board = self.pack_power_board_info(ipmbaddr=i+1, sensornum=5)
            packet[offset:offset+128] = power_board
            offset += 128
        
        # board[10] (1300字节，负载槽1,2,3,4,6,7,9,10,11,12)
        slot_numbers = [1, 2, 3, 4, 6, 7, 9, 10, 11, 12]
        for slot_num in slot_numbers:
            slot_board = self.pack_slot_board_info(
                ipmbaddr=slot_num,
                prst=1,
                sensornum=5
            )
            packet[offset:offset+130] = slot_board
            offset += 130
        
        # tail (2字节)
        struct.pack_into('<H', packet, offset, self.TAIL)
        
        return bytes(packet)
    
    def send_packet(self):
        """发送一个数据包"""
        packet = self.build_udp_packet()
        self.sock.sendto(packet, (self.MULTICAST_ADDR, self.MULTICAST_PORT))
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] "
              f"发送数据包 #{self.seqnum}, 大小: {len(packet)} 字节, "
              f"目标: {self.MULTICAST_ADDR}:{self.MULTICAST_PORT}")
        self.seqnum = (self.seqnum % 65535) + 1
    
    def run(self, count=None):
        """
        运行模拟器
        
        Args:
            count: 发送次数，None 表示无限循环
        """
        print(f"BMC UDP 模拟器启动")
        print(f"组播地址: {self.MULTICAST_ADDR}:{self.MULTICAST_PORT}")
        print(f"机箱号: {self.box_id}")
        print(f"模块设备号: 0x{self.module_type:04X}")
        print(f"发送间隔: {self.interval} 秒")
        print(f"发送次数: {'无限' if count is None else count}")
        print("-" * 60)
        
        try:
            sent = 0
            while count is None or sent < count:
                self.send_packet()
                sent += 1
                if count is None or sent < count:
                    time.sleep(self.interval)
        except KeyboardInterrupt:
            print("\n收到中断信号，停止发送")
        finally:
            self.sock.close()
            print(f"总共发送 {sent} 个数据包")


def main():
    parser = argparse.ArgumentParser(description='BMC UDP 数据模拟器')
    parser.add_argument('--box-id', type=int, default=1, 
                       help='机箱号 (默认: 1)')
    parser.add_argument('--module-type', type=int, default=0x0001,
                       help='模块设备号 (默认: 0x0001)')
    parser.add_argument('--interval', type=float, default=1.0,
                       help='发送间隔，秒 (默认: 1.0)')
    parser.add_argument('--count', type=int, default=None,
                       help='发送次数，不指定则无限循环 (默认: None)')
    
    args = parser.parse_args()
    
    simulator = BMCUdpSimulator(
        box_id=args.box_id,
        module_type=args.module_type,
        interval=args.interval
    )
    
    simulator.run(count=args.count)


if __name__ == '__main__':
    main()

