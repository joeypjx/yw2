# 历史监控数据SQL查询文档

本文档详细说明获取历史监控数据时使用的SQL语句，包括CPU、内存、网络、磁盘、GPU和BMC传感器等各类资源的查询语句。

## 目录

1. [查询方式概述](#查询方式概述)
2. [CPU指标查询](#cpu指标查询)
3. [内存指标查询](#内存指标查询)
4. [网络指标查询](#网络指标查询)
5. [磁盘指标查询](#磁盘指标查询)
6. [GPU指标查询](#gpu指标查询)
7. [BMC传感器查询](#bmc传感器查询)
8. [动态Bucket大小计算](#动态bucket大小计算)

## 查询方式概述

系统提供两种查询历史监控数据的方式：

### 1. 基于时间范围（Duration）的查询

使用PostgreSQL interval格式指定时间范围，固定使用10秒的bucket大小进行聚合。

**函数签名**：
```cpp
MetricsSeries queryMetricsSeries(
    const std::string& host_ip,
    const std::string& duration,  // 如 "5 minutes", "1 hour"
    const std::vector<std::string>& kinds
);
```

**特点**：
- Bucket大小固定为10秒
- 使用 `time_bucket_gapfill` 函数进行时间分桶和缺失值填充
- 查询最近N秒/分钟/小时的数据

### 2. 基于时间戳范围的查询

使用开始和结束时间戳（秒级）指定时间范围，根据时间范围动态调整bucket大小。

**函数签名**：
```cpp
MetricsSeries queryMetricsSeries(
    const std::string& host_ip,
    std::int64_t start_time,  // 开始时间戳（秒）
    std::int64_t end_time,    // 结束时间戳（秒）
    const std::vector<std::string>& kinds
);
```

**特点**：
- Bucket大小根据时间范围动态调整（目标数据点数量在300-1000之间）
- 使用 `time_bucket_gapfill` 函数进行时间分桶和缺失值填充
- 支持查询任意时间范围的数据

## CPU指标查询

### 基于Duration的查询

**SQL语句**：
```sql
SELECT 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(usage_percent, 0) as usage_percent, 
    COALESCE(load_avg_1m, 0) as load_avg_1m, 
    COALESCE(load_avg_5m, 0) as load_avg_5m, 
    COALESCE(load_avg_15m, 0) as load_avg_15m, 
    COALESCE(core_count, 0)::int as core_count, 
    COALESCE(core_allocated, 0)::int as core_allocated, 
    COALESCE(temperature, 0) as temperature, 
    COALESCE(voltage, 0) as voltage, 
    COALESCE(current, 0) as current, 
    COALESCE(power, 0) as power 
FROM ( 
    SELECT 
        time_bucket_gapfill('10 seconds', time, now() - $2::interval, now()) AS bucket, 
        AVG(usage_percent) as usage_percent, 
        AVG(load_avg_1m) as load_avg_1m, 
        AVG(load_avg_5m) as load_avg_5m, 
        AVG(load_avg_15m) as load_avg_15m, 
        ROUND(AVG(core_count)) as core_count, 
        ROUND(AVG(core_allocated)) as core_allocated, 
        AVG(temperature) as temperature, 
        AVG(voltage) as voltage, 
        AVG(current) as current, 
        AVG(power) as power 
    FROM resource_cpu 
    WHERE host_ip = $1::inet 
      AND time >= now() - $2::interval 
      AND time <= now() 
    GROUP BY bucket 
) AS gapfilled_data 
ORDER BY ts ASC
```

**参数说明**：
- `$1`: 节点IP地址（host_ip）
- `$2`: 时间范围（duration，如 "5 minutes"）

**查询字段**：
- `ts`: 时间戳（Unix秒级时间戳）
- `usage_percent`: CPU使用率百分比
- `load_avg_1m`: 1分钟平均负载
- `load_avg_5m`: 5分钟平均负载
- `load_avg_15m`: 15分钟平均负载
- `core_count`: CPU核心总数
- `core_allocated`: 已分配核心数
- `temperature`: CPU温度
- `voltage`: CPU电压
- `current`: CPU电流
- `power`: CPU功耗

### 基于时间戳范围的查询

**SQL语句**：
```sql
SELECT 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(usage_percent, 0) as usage_percent, 
    COALESCE(load_avg_1m, 0) as load_avg_1m, 
    COALESCE(load_avg_5m, 0) as load_avg_5m, 
    COALESCE(load_avg_15m, 0) as load_avg_15m, 
    COALESCE(core_count, 0)::int as core_count, 
    COALESCE(core_allocated, 0)::int as core_allocated, 
    COALESCE(temperature, 0) as temperature, 
    COALESCE(voltage, 0) as voltage, 
    COALESCE(current, 0) as current, 
    COALESCE(power, 0) as power 
FROM ( 
    SELECT 
        time_bucket_gapfill('动态bucket大小', time, to_timestamp($2), to_timestamp($3)) AS bucket, 
        AVG(usage_percent) as usage_percent, 
        AVG(load_avg_1m) as load_avg_1m, 
        AVG(load_avg_5m) as load_avg_5m, 
        AVG(load_avg_15m) as load_avg_15m, 
        ROUND(AVG(core_count)) as core_count, 
        ROUND(AVG(core_allocated)) as core_allocated, 
        AVG(temperature) as temperature, 
        AVG(voltage) as voltage, 
        AVG(current) as current, 
        AVG(power) as power 
    FROM resource_cpu 
    WHERE host_ip = $1::inet 
      AND time >= to_timestamp($2) 
      AND time <= to_timestamp($3) 
    GROUP BY bucket 
) AS gapfilled_data 
ORDER BY ts ASC
```

**参数说明**：
- `$1`: 节点IP地址（host_ip）
- `$2`: 开始时间戳（秒）
- `$3`: 结束时间戳（秒）
- `动态bucket大小`: 根据时间范围计算，详见[动态Bucket大小计算](#动态bucket大小计算)

## 内存指标查询

### 基于Duration的查询

**SQL语句**：
```sql
SELECT 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(total, 0)::bigint as total, 
    COALESCE(used, 0)::bigint as used, 
    COALESCE(free, 0)::bigint as free, 
    COALESCE(usage_percent, 0) as usage_percent 
FROM ( 
    SELECT 
        time_bucket_gapfill('10 seconds', time, now() - $2::interval, now()) AS bucket, 
        ROUND(AVG(total)) as total, 
        ROUND(AVG(used)) as used, 
        ROUND(AVG(free)) as free, 
        AVG(usage_percent) as usage_percent 
    FROM resource_memory 
    WHERE host_ip = $1::inet 
      AND time >= now() - $2::interval 
      AND time <= now() 
    GROUP BY bucket 
) AS gapfilled_data 
ORDER BY ts ASC
```

**查询字段**：
- `ts`: 时间戳
- `total`: 总内存（字节）
- `used`: 已使用内存（字节）
- `free`: 空闲内存（字节）
- `usage_percent`: 内存使用率百分比

### 基于时间戳范围的查询

**SQL语句**：
```sql
SELECT 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(total, 0)::bigint as total, 
    COALESCE(used, 0)::bigint as used, 
    COALESCE(free, 0)::bigint as free, 
    COALESCE(usage_percent, 0) as usage_percent 
FROM ( 
    SELECT 
        time_bucket_gapfill('动态bucket大小', time, to_timestamp($2), to_timestamp($3)) AS bucket, 
        ROUND(AVG(total)) as total, 
        ROUND(AVG(used)) as used, 
        ROUND(AVG(free)) as free, 
        AVG(usage_percent) as usage_percent 
    FROM resource_memory 
    WHERE host_ip = $1::inet 
      AND time >= to_timestamp($2) 
      AND time <= to_timestamp($3) 
    GROUP BY bucket 
) AS gapfilled_data 
ORDER BY ts ASC
```

## 网络指标查询

### 基于Duration的查询

**SQL语句**：
```sql
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
interface_dims AS (
  SELECT DISTINCT interface FROM resource_network
  WHERE host_ip = $1::inet
    AND time >= now() - $2::interval AND time <= now()
)
SELECT
    dims.interface,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    COALESCE(ROUND(AVG(metrics.rx_bytes)), 0)::bigint AS rx_bytes,
    COALESCE(ROUND(AVG(metrics.tx_bytes)), 0)::bigint AS tx_bytes,
    COALESCE(ROUND(AVG(metrics.rx_packets)), 0)::bigint AS rx_packets,
    COALESCE(ROUND(AVG(metrics.tx_packets)), 0)::bigint AS tx_packets,
    COALESCE(ROUND(AVG(metrics.rx_errors)), 0)::bigint AS rx_errors,
    COALESCE(ROUND(AVG(metrics.tx_errors)), 0)::bigint AS tx_errors,
    COALESCE(ROUND(AVG(metrics.rx_rate)), 0)::bigint AS rx_rate,
    COALESCE(ROUND(AVG(metrics.tx_rate)), 0)::bigint AS tx_rate
FROM
    interface_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    resource_network AS metrics
ON
    metrics.interface = dims.interface
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics.time) = buckets.bucket
    AND metrics.time >= now() - $2::interval AND metrics.time <= now()
GROUP BY
    dims.interface, buckets.bucket
ORDER BY
    dims.interface, ts ASC
```

**查询逻辑**：
1. `bucket_series`: 生成所有10秒时间桶的序列
2. `interface_dims`: 获取该节点在时间范围内的所有网络接口
3. `CROSS JOIN`: 将接口维度与时间桶进行笛卡尔积，确保所有接口和时间桶都有数据
4. `LEFT JOIN`: 左连接实际数据，缺失值用0填充
5. 按接口和时间桶分组聚合

**查询字段**：
- `interface`: 网络接口名称
- `ts`: 时间戳
- `rx_bytes`: 接收字节数
- `tx_bytes`: 发送字节数
- `rx_packets`: 接收包数
- `tx_packets`: 发送包数
- `rx_errors`: 接收错误数
- `tx_errors`: 发送错误数
- `rx_rate`: 接收速率（字节/秒）
- `tx_rate`: 发送速率（字节/秒）

### 基于时间戳范围的查询

**SQL语句**：
```sql
SELECT 
    interface, 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(ROUND(AVG(rx_bytes)), 0)::bigint AS rx_bytes, 
    COALESCE(ROUND(AVG(tx_bytes)), 0)::bigint AS tx_bytes, 
    COALESCE(ROUND(AVG(rx_packets)), 0)::bigint AS rx_packets, 
    COALESCE(ROUND(AVG(tx_packets)), 0)::bigint AS tx_packets, 
    COALESCE(ROUND(AVG(rx_errors)), 0)::bigint AS rx_errors, 
    COALESCE(ROUND(AVG(tx_errors)), 0)::bigint AS tx_errors, 
    COALESCE(AVG(rx_rate), 0) AS rx_rate, 
    COALESCE(AVG(tx_rate), 0) AS tx_rate, 
    COALESCE(AVG(rx_drop_rate), 0) AS rx_drop_rate, 
    COALESCE(AVG(tx_drop_rate), 0) AS tx_drop_rate, 
    COALESCE(MODE() WITHIN GROUP (ORDER BY state), 0)::int AS state 
FROM ( 
    SELECT 
        time_bucket_gapfill('动态bucket大小', time, to_timestamp($2), to_timestamp($3)) AS bucket, 
        interface, 
        rx_bytes, tx_bytes, rx_packets, tx_packets, 
        rx_errors, tx_errors, rx_rate, tx_rate, 
        rx_drop_rate, tx_drop_rate, state 
    FROM resource_network 
    WHERE host_ip = $1::inet 
      AND time >= to_timestamp($2) AND time <= to_timestamp($3) 
) AS gapfilled 
GROUP BY interface, bucket 
ORDER BY interface, ts ASC
```

**注意**：
- 使用 `time_bucket_gapfill` 替代 CROSS JOIN，性能更好
- 包含 `rx_drop_rate`、`tx_drop_rate`、`state` 字段
- 使用 `MODE() WITHIN GROUP` 获取状态字段的众数

## 磁盘指标查询

### 基于Duration的查询

**SQL语句**：
```sql
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
disk_dims AS (
  SELECT DISTINCT device, mount_point FROM resource_disk
  WHERE host_ip = $1::inet
    AND time >= now() - $2::interval AND time <= now()
)
SELECT
    dims.device,
    dims.mount_point,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    COALESCE(ROUND(AVG(metrics.total)), 0)::bigint AS total,
    COALESCE(ROUND(AVG(metrics.used)), 0)::bigint AS used,
    COALESCE(ROUND(AVG(metrics.free)), 0)::bigint AS free,
    COALESCE(AVG(metrics.usage_percent), 0) AS usage_percent
FROM
    disk_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    resource_disk AS metrics
ON
    metrics.device = dims.device
    AND metrics.mount_point = dims.mount_point
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics.time) = buckets.bucket
    AND metrics.time >= now() - $2::interval AND metrics.time <= now()
GROUP BY
    dims.device, dims.mount_point, buckets.bucket
ORDER BY
   dims.device, ts ASC
```

**查询逻辑**：
1. `bucket_series`: 生成所有10秒时间桶的序列
2. `disk_dims`: 获取该节点在时间范围内的所有磁盘设备（device + mount_point组合）
3. `CROSS JOIN`: 将磁盘维度与时间桶进行笛卡尔积
4. `LEFT JOIN`: 左连接实际数据，缺失值用0填充
5. 按设备、挂载点和时间桶分组聚合

**查询字段**：
- `device`: 磁盘设备名称
- `mount_point`: 挂载点路径
- `ts`: 时间戳
- `total`: 总容量（字节）
- `used`: 已使用容量（字节）
- `free`: 空闲容量（字节）
- `usage_percent`: 使用率百分比

### 基于时间戳范围的查询

**SQL语句**：
```sql
SELECT 
    device, 
    mount_point, 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(ROUND(AVG(total)), 0)::bigint AS total, 
    COALESCE(ROUND(AVG(used)), 0)::bigint AS used, 
    COALESCE(ROUND(AVG(free)), 0)::bigint AS free, 
    COALESCE(AVG(usage_percent), 0) AS usage_percent 
FROM ( 
    SELECT 
        time_bucket_gapfill('动态bucket大小', time, to_timestamp($2), to_timestamp($3)) AS bucket, 
        device, mount_point, total, used, free, usage_percent 
    FROM resource_disk 
    WHERE host_ip = $1::inet 
      AND time >= to_timestamp($2) AND time <= to_timestamp($3) 
) AS gapfilled 
GROUP BY device, mount_point, bucket 
ORDER BY device, ts ASC
```

## GPU指标查询

### 基于Duration的查询

**SQL语句**：
```sql
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
gpu_dims AS (
  SELECT DISTINCT gpu_index, name FROM resource_gpu
  WHERE host_ip = $1::inet
    AND time >= now() - $2::interval AND time <= now()
)
SELECT
    dims.gpu_index,
    dims.name,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    COALESCE(AVG(metrics.compute_usage), 0) AS compute_usage,
    COALESCE(AVG(metrics.mem_usage), 0) AS mem_usage,
    COALESCE(ROUND(AVG(metrics.mem_used)), 0)::bigint AS mem_used,
    COALESCE(ROUND(AVG(metrics.mem_total)), 0)::bigint AS mem_total,
    COALESCE(AVG(metrics.temperature), 0) AS temperature,
    COALESCE(AVG(metrics.power), 0) AS power,
    COALESCE(ROUND(AVG(metrics.free)), 0)::integer AS free
FROM
    gpu_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    resource_gpu AS metrics
ON
    metrics.gpu_index = dims.gpu_index
    AND metrics.name = dims.name
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics.time) = buckets.bucket
    AND metrics.time >= now() - $2::interval AND metrics.time <= now()
GROUP BY
    dims.gpu_index, dims.name, buckets.bucket
ORDER BY
    dims.gpu_index, ts ASC
```

**查询逻辑**：
1. `bucket_series`: 生成所有10秒时间桶的序列
2. `gpu_dims`: 获取该节点在时间范围内的所有GPU（gpu_index + name组合）
3. `CROSS JOIN`: 将GPU维度与时间桶进行笛卡尔积
4. `LEFT JOIN`: 左连接实际数据，缺失值用0填充
5. 按GPU索引、名称和时间桶分组聚合

**查询字段**：
- `gpu_index`: GPU索引号
- `name`: GPU名称
- `ts`: 时间戳
- `compute_usage`: 计算使用率百分比
- `mem_usage`: 显存使用率百分比
- `mem_used`: 已使用显存（字节）
- `mem_total`: 总显存（字节）
- `temperature`: GPU温度
- `power`: GPU功耗
- `free`: GPU空闲状态（0或1）

### 基于时间戳范围的查询

**SQL语句**：
```sql
SELECT 
    gpu_index, 
    name, 
    EXTRACT(EPOCH FROM bucket)::bigint AS ts, 
    COALESCE(AVG(compute_usage), 0) AS compute_usage, 
    COALESCE(AVG(mem_usage), 0) AS mem_usage, 
    COALESCE(ROUND(AVG(mem_used)), 0)::bigint AS mem_used, 
    COALESCE(ROUND(AVG(mem_total)), 0)::bigint AS mem_total, 
    COALESCE(AVG(temperature), 0) AS temperature, 
    COALESCE(AVG(power), 0) AS power, 
    COALESCE(ROUND(AVG(free)), 0)::integer AS free 
FROM ( 
    SELECT 
        time_bucket_gapfill('动态bucket大小', time, to_timestamp($2), to_timestamp($3)) AS bucket, 
        gpu_index, name, compute_usage, mem_usage, mem_used, mem_total, temperature, power, free 
    FROM resource_gpu 
    WHERE host_ip = $1::inet 
      AND time >= to_timestamp($2) AND time <= to_timestamp($3) 
) AS gapfilled 
GROUP BY gpu_index, name, bucket 
ORDER BY gpu_index, ts ASC
```

## BMC传感器查询

### 历史传感器数据查询

**SQL语句**：
```sql
WITH bucket_series AS (
  SELECT generate_series(
    time_bucket('10 seconds', now() - $2::interval),
    time_bucket('10 seconds', now()),
    '10 seconds'::interval
  ) AS bucket
),
sensor_dims AS (
  SELECT DISTINCT sensorname, sensorseq, sensortype FROM bmc_sensor
  WHERE host_ip = $1::inet
    AND "time" >= now() - $2::interval AND "time" <= now()
)
SELECT
    dims.sensorname,
    EXTRACT(EPOCH FROM buckets.bucket)::bigint AS ts,
    $1::inet AS host_ip,
    dims.sensorseq,
    dims.sensortype,
    COALESCE(ROUND(AVG(metrics.sensorvalue_L)), 0)::smallint AS sensorvalue_L,
    COALESCE(ROUND(AVG(metrics.sensorvalue_H)), 0)::smallint AS sensorvalue_H,
    COALESCE(ROUND(AVG(metrics.sensoralmtype)), 0)::smallint AS sensoralmtype
FROM
    sensor_dims AS dims
CROSS JOIN
    bucket_series AS buckets
LEFT JOIN
    bmc_sensor AS metrics
ON
    metrics.sensorname = dims.sensorname
    AND metrics.sensorseq = dims.sensorseq
    AND metrics.sensortype = dims.sensortype
    AND metrics.host_ip = $1::inet
    AND time_bucket('10 seconds', metrics."time") = buckets.bucket
    AND metrics."time" >= now() - $2::interval AND metrics."time" <= now()
GROUP BY
    dims.sensorname, dims.sensorseq, dims.sensortype, buckets.bucket
ORDER BY
    dims.sensorname, ts ASC
```

**参数说明**：
- `$1`: 节点IP地址（host_ip）
- `$2`: 时间范围（duration，如 "5 minutes"）

**查询字段**：
- `sensorname`: 传感器名称
- `ts`: 时间戳（Unix秒级时间戳）
- `host_ip`: 节点IP地址
- `sensorseq`: 传感器序号
- `sensortype`: 传感器类型
- `sensorvalue_L`: 传感器数值低字节（小数部分）
- `sensorvalue_H`: 传感器数值高字节（整数部分）
- `sensoralmtype`: 告警类型

**传感器值计算**：
```cpp
sensor_value = sensorvalue_H + sensorvalue_L * 0.01
```

### 最新传感器数据查询

**SQL语句**：
```sql
SELECT DISTINCT ON (sensorname)
    sensorname,
    EXTRACT(EPOCH FROM "time")::bigint AS timestamp,
    host_ip::text,
    sensorseq,
    sensortype,
    sensorvalue_L,
    sensorvalue_H,
    sensoralmtype
FROM bmc_sensor
WHERE host_ip = $1::inet
  AND "time" >= NOW() - INTERVAL '5 minutes'
ORDER BY sensorname, "time" DESC
```

**参数说明**：
- `$1`: 节点IP地址（host_ip）

**查询逻辑**：
- 使用 `DISTINCT ON (sensorname)` 获取每个传感器的最新一条记录
- 只查询最近5分钟的数据，避免扫描整个表
- 按传感器名称和时间降序排序

## 动态Bucket大小计算

对于基于时间戳范围的查询，系统会根据时间范围动态调整bucket大小，以保持数据点数量在合理范围内（300-1000点）。

### 计算逻辑

```cpp
// 计算时间范围（秒）
const std::int64_t duration_seconds = end_time - start_time;

// 计算最优 bucket 大小（秒）
constexpr std::int64_t TARGET_POINTS_OPTIMAL = 500;  // 最优目标点数
std::int64_t bucket_seconds = duration_seconds / TARGET_POINTS_OPTIMAL;

// 将 bucket 大小调整为合理的值（TimescaleDB 推荐使用标准时间单位）
std::string bucket_interval;
if (bucket_seconds <= 10) {
    bucket_interval = "'10 seconds'";
} else if (bucket_seconds <= 30) {
    bucket_interval = "'30 seconds'";
} else if (bucket_seconds <= 60) {
    bucket_interval = "'1 minute'";
} else if (bucket_seconds <= 300) {
    bucket_interval = "'5 minutes'";
} else if (bucket_seconds <= 600) {
    bucket_interval = "'10 minutes'";
} else if (bucket_seconds <= 1800) {
    bucket_interval = "'30 minutes'";
} else if (bucket_seconds <= 3600) {
    bucket_interval = "'1 hour'";
} else if (bucket_seconds <= 7200) {
    bucket_interval = "'2 hours'";
} else {
    bucket_interval = "'6 hours'";
}
```

### Bucket大小映射表

| 计算出的bucket_seconds | 实际使用的bucket_interval |
|----------------------|-------------------------|
| ≤ 10秒 | '10 seconds' |
| 11-30秒 | '30 seconds' |
| 31-60秒 | '1 minute' |
| 61-300秒 | '5 minutes' |
| 301-600秒 | '10 minutes' |
| 601-1800秒 | '30 minutes' |
| 1801-3600秒 | '1 hour' |
| 3601-7200秒 | '2 hours' |
| > 7200秒 | '6 hours' |

### 示例

假设查询时间范围为1小时（3600秒）：
- `bucket_seconds = 3600 / 500 = 7.2秒`
- 实际使用 `'10 seconds'` bucket

假设查询时间范围为1天（86400秒）：
- `bucket_seconds = 86400 / 500 = 172.8秒`
- 实际使用 `'5 minutes'` bucket

假设查询时间范围为1个月（2592000秒）：
- `bucket_seconds = 2592000 / 500 = 5184秒`
- 实际使用 `'2 hours'` bucket

## 数据后处理

### 删除首尾不完整数据点

对于所有查询，系统会删除首尾不完整的数据点：

```cpp
// 删除首尾元素（gapfill可能产生的不完整数据点）
if (points.size() >= 2) {
    points.erase(points.begin());      // 删除第一个元素
    points.erase(points.end() - 1);    // 删除最后一个元素
} else if (points.size() == 1) {
    // 如果只有1个数据点，很可能是gapfill产生的，直接清空
    points.clear();
}
```

**原因**：
- 首尾时间桶可能只包含部分数据
- 为了数据准确性，将其删除

## 性能优化

### 1. 使用TimescaleDB特性

- **time_bucket_gapfill**: 自动进行时间分桶和缺失值填充
- **Hypertable**: 按时间分区，提高查询性能
- **压缩策略**: 7天后自动压缩，节省存储空间

### 2. 索引优化

所有表都创建了相应的索引：
- `(host_ip, time)`: 主查询索引
- `(host_ip, time, interface)`: 网络表索引
- `(host_ip, time, device, mount_point)`: 磁盘表索引
- `(host_ip, time, gpu_index)`: GPU表索引

### 3. 连接池管理

使用PostgreSQL连接池管理数据库连接，提高并发性能：
- 最小连接数：2
- 最大连接数：10

### 4. 查询优化

- **预分配内存**: 使用 `reserve()` 预分配向量容量
- **参数化查询**: 使用参数化查询避免SQL注入，提高性能
- **只查询必要字段**: 避免查询不必要的数据

## 总结

历史监控数据查询使用TimescaleDB的时序数据库特性，通过时间分桶和聚合实现高效的数据查询。主要特点：

1. **两种查询方式**：基于duration的固定bucket查询和基于时间戳范围的动态bucket查询
2. **自动缺失值填充**：使用 `time_bucket_gapfill` 自动填充缺失的时间点
3. **动态bucket调整**：根据时间范围自动调整bucket大小，保持合理的数据点数量
4. **多维度支持**：支持按网络接口、磁盘设备、GPU索引等多维度查询
5. **性能优化**：使用索引、连接池、预分配内存等技术提高查询性能
