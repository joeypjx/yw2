# BMC 模块数据库表结构文档

本文档描述了 BMC 模块使用的所有数据库表结构。所有表均为 TimescaleDB 时序表（hypertable）。

## 1. bmc_fan - 风扇表

用于记录机箱风扇的运行状态和速度信息。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳（来自 UdpInfo.timestamp） |
| boxid | INTEGER | NOT NULL | 机箱ID（来自 UdpInfo.boxid） |
| fanseq | SMALLINT | | 风扇序号（来自 UdpFanInfo.fanseq），0xFF 表示无此风扇 |
| fanmode | SMALLINT | | 风扇模式（来自 UdpFanInfo.fanmode，原始字节，不拆高低位） |
| fanspeed | INTEGER | | 风扇转速（来自 UdpFanInfo.fanspeed） |

**说明**：
- 每个机箱最多有 6 个风扇，每个风扇一条记录
- `fanseq` 为 0xFF 时表示该风扇不存在，会被跳过不保存
- `boxid` 用于标识不同的机箱

## 2. bmc_sensor - 传感器表

用于记录负载槽板卡上的传感器数据，包括温度、电压、电流等传感器信息。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳（来自 UdpInfo.timestamp） |
| host_ip | INET | NOT NULL | 节点IP地址（由 boxid 和 slot_id 计算得出） |
| sensorseq | SMALLINT | | 传感器序号（来自 UdpSensorInfo.sensorseq），0xFF 表示无此传感器 |
| sensortype | SMALLINT | | 传感器类型（来自 UdpSensorInfo.sensortype） |
| sensorname | TEXT | | 传感器名称（来自 UdpSensorInfo.sensorname[6] 转字符串） |
| sensorvalue_L | SMALLINT | | 传感器值低字节（来自 UdpSensorInfo.sensorvalue_L） |
| sensorvalue_H | SMALLINT | | 传感器值高字节（来自 UdpSensorInfo.sensorvalue_H） |
| sensoralmtype | SMALLINT | | 传感器告警类型（来自 UdpSensorInfo.sensoralmtype） |

**说明**：
- 每个负载槽最多有 8 个传感器，每个传感器一条记录
- `sensorseq` 为 0xFF 时表示该传感器不存在，会被跳过不保存
- `host_ip` 通过 `boxid` 和 `slot_id` 映射计算得出，格式为 `192.168.{network_id}.{host_id}`
- 传感器实际值计算公式：`sensor_value = sensorvalue_H + sensorvalue_L * 0.01`
- 负载槽顺序映射：槽1、槽2、槽3、槽4、槽6、槽7、槽9、槽10、槽11、槽12（共10个负载槽）
- 电源模块（dyboard）的传感器数据当前不保存

## 表关系说明

- `bmc_fan` 表通过 `boxid` 和 `time` 字段进行关联和查询
- `bmc_sensor` 表通过 `host_ip` 和 `time` 字段进行关联和查询
- `host_ip` 字段与 monitor 模块的资源表通过 `host_ip` 进行关联
- `boxid` 用于标识不同的机箱，一个机箱可以包含多个负载槽

## 数据来源

### bmc_fan 表
- 数据来源：UDP 协议包 `UdpInfo` 中的 `fan[6]` 数组
- 每个机箱最多 6 个风扇
- 保存时使用当前时间戳（`now()`）

### bmc_sensor 表
- 数据来源：UDP 协议包 `UdpInfo` 中的 `board[10]` 数组（负载槽）
- 每个负载槽最多 8 个传感器
- 只保存负载槽在位（`prst != 0`）的传感器数据
- 保存时使用当前时间戳（`now()`）

## 注意事项

1. 所有表均为 TimescaleDB hypertable，按 `time` 字段分区
2. `bmc_fan` 表配置了压缩策略（7天后压缩）和保留策略（30天后删除）
3. `bmc_sensor` 表配置了压缩策略（7天后压缩）和保留策略（90天后删除）
4. 所有表都创建了相应的索引以优化查询性能
5. `host_ip` 字段使用 PostgreSQL 的 `INET` 类型，支持 IPv4 和 IPv6 地址
6. `bmc_fan` 表使用 `boxid` 作为分段键（segmentby），`bmc_sensor` 表使用 `host_ip` 作为分段键
7. 传感器序号为 0xFF 的记录会被跳过，不会保存到数据库
8. 负载槽不在位（`prst == 0`）的传感器数据不会被保存

