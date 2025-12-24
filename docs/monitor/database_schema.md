# Monitor 模块数据库表结构文档

本文档描述了 monitor 模块使用的所有数据库表结构。所有表均为 TimescaleDB 时序表（hypertable）。

## 1. resource_alive - 节点心跳表

用于记录节点的存活状态，每次资源上报时插入一条记录。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| alive | SMALLINT | NOT NULL | 存活状态，通常为 1 |

## 2. resource_cpu - CPU 资源表

用于记录节点的 CPU 资源使用情况。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| usage_percent | DOUBLE PRECISION | | CPU 使用率百分比 |
| load_avg_1m | DOUBLE PRECISION | | 1分钟平均负载 |
| load_avg_5m | DOUBLE PRECISION | | 5分钟平均负载 |
| load_avg_15m | DOUBLE PRECISION | | 15分钟平均负载 |
| core_count | INTEGER | | CPU 核心总数 |
| core_allocated | INTEGER | | 已分配的核心数 |
| temperature | DOUBLE PRECISION | | CPU 温度 |
| voltage | DOUBLE PRECISION | | CPU 电压 |
| current | DOUBLE PRECISION | | CPU 电流 |
| power | DOUBLE PRECISION | | CPU 功耗 |

## 3. resource_memory - 内存资源表

用于记录节点的内存资源使用情况。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| total | BIGINT | | 总内存大小（字节） |
| used | BIGINT | | 已使用内存大小（字节） |
| free | BIGINT | | 空闲内存大小（字节） |
| usage_percent | DOUBLE PRECISION | | 内存使用率百分比 |

## 4. resource_network - 网络资源表

用于记录节点的网络接口资源使用情况，每行代表一个网络接口。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| interface | TEXT | NOT NULL | 网络接口名称 |
| rx_bytes | BIGINT | | 接收字节数 |
| tx_bytes | BIGINT | | 发送字节数 |
| rx_packets | BIGINT | | 接收数据包数 |
| tx_packets | BIGINT | | 发送数据包数 |
| rx_errors | BIGINT | | 接收错误数 |
| tx_errors | BIGINT | | 发送错误数 |
| rx_rate | DOUBLE PRECISION | | 接收速率（字节/秒） |
| tx_rate | DOUBLE PRECISION | | 发送速率（字节/秒） |
| rx_drop_rate | DOUBLE PRECISION | | 接收丢包率 |
| tx_drop_rate | DOUBLE PRECISION | | 发送丢包率 |
| state | INTEGER | | 接口状态 |

## 5. resource_disk - 磁盘资源表

用于记录节点的磁盘资源使用情况，每行代表一个磁盘设备或挂载点。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| device | TEXT | NOT NULL | 磁盘设备名称 |
| mount_point | TEXT | NOT NULL | 挂载点路径 |
| total | BIGINT | | 总容量（字节） |
| used | BIGINT | | 已使用容量（字节） |
| free | BIGINT | | 空闲容量（字节） |
| usage_percent | DOUBLE PRECISION | | 使用率百分比 |

## 6. resource_gpu - GPU 资源表

用于记录节点的 GPU 资源使用情况，每行代表一块 GPU。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| gpu_index | INTEGER | NOT NULL | GPU 索引号 |
| name | TEXT | | GPU 名称 |
| compute_usage | DOUBLE PRECISION | | 计算使用率 |
| mem_usage | DOUBLE PRECISION | | 内存使用率 |
| mem_used | BIGINT | | 已使用显存（字节） |
| mem_total | BIGINT | | 总显存（字节） |
| temperature | DOUBLE PRECISION | | GPU 温度 |
| power | DOUBLE PRECISION | | GPU 功耗 |
| free | INTEGER | | GPU 空闲状态（1表示空闲，0表示占用） |
| gpu_allocated | INTEGER | | 已分配的 GPU 数量 |
| gpu_num | INTEGER | | 节点 GPU 总数 |

## 7. resource_component - 组件资源表

用于记录节点上运行的组件（容器）的资源使用情况，每行代表一个组件实例。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| instance_id | TEXT | NOT NULL | 实例ID |
| uuid | TEXT | NOT NULL | 组件UUID |
| idx | INTEGER | NOT NULL | 组件索引 |
| name | TEXT | | 组件名称 |
| container_id | TEXT | | 容器ID |
| state | TEXT | | 组件状态 |
| type | TEXT | | 组件类型（如 "docker"） |
| cpu_load | DOUBLE PRECISION | | CPU 负载 |
| mem_used | BIGINT | | 已使用内存（字节） |
| mem_limit | BIGINT | | 内存限制（字节） |
| mem_usage | DOUBLE PRECISION | | 内存使用率（百分比） |
| net_tx | BIGINT | | 网络发送字节数 |
| net_rx | BIGINT | | 网络接收字节数 |
| net_rx_rate | DOUBLE PRECISION | | 网络接收速率（字节/秒） |
| net_tx_rate | DOUBLE PRECISION | | 网络发送速率（字节/秒） |

## 8. resource_raw - 原始报文表（可选）

用于存储原始的资源上报报文，便于后续分析和调试。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| time | TIMESTAMPTZ | NOT NULL | 时间戳 |
| host_ip | INET | NOT NULL | 节点IP地址 |
| payload | JSONB | NOT NULL | 原始JSON报文内容 |

## 表关系说明

- 所有表都通过 `time` 和 `host_ip` 字段进行关联
- `resource_alive` 表用于记录节点心跳，与其他资源表的时间戳对应
- `resource_network`、`resource_disk`、`resource_gpu` 表通过额外的维度字段（`interface`、`device`、`gpu_index`）区分不同的资源实例
- `resource_component` 表通过 `instance_id` 和 `uuid` 标识不同的组件实例

## 注意事项

1. 所有表均为 TimescaleDB hypertable，按 `time` 字段分区
2. 所有表都配置了压缩策略（7天后压缩）和保留策略（90天后删除）
3. 所有表都创建了相应的索引以优化查询性能
4. `host_ip` 字段使用 PostgreSQL 的 `INET` 类型，支持 IPv4 和 IPv6 地址

