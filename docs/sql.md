下面给出一个简洁、可落地的 TimescaleDB 设计方案，覆盖表结构、超表、索引、压缩与保留、连续聚合，以及写入建议。

### 一、启用扩展
```sql
CREATE EXTENSION IF NOT EXISTS timescaledb;
```

### 二、时间与标签设计
- 统一时间列为 `time TIMESTAMPTZ NOT NULL`（写入时用服务器接收时间）。
- 维度（标签）：
  - 主机：`host_ip INET`（如果你更习惯字符串，也可用 TEXT）。
  - 网络接口/磁盘设备/GPU 索引/组件 ID 等作为次级标签列。

### 三、资源表结构（窄表，按资源域拆分）
1) CPU
```sql
CREATE TABLE resource_cpu (
  time            TIMESTAMPTZ NOT NULL,
  host_ip         INET        NOT NULL,
  usage_percent   DOUBLE PRECISION,
  load_avg_1m     DOUBLE PRECISION,
  load_avg_5m     DOUBLE PRECISION,
  load_avg_15m    DOUBLE PRECISION,
  core_count      INTEGER,
  core_allocated  INTEGER,
  temperature     DOUBLE PRECISION,
  voltage         DOUBLE PRECISION,
  current         DOUBLE PRECISION,
  power           DOUBLE PRECISION
);
SELECT create_hypertable('resource_cpu','time','host_ip',4, if_not_exists => TRUE);
```

2) Memory
```sql
CREATE TABLE resource_memory (
  time            TIMESTAMPTZ NOT NULL,
  host_ip         INET        NOT NULL,
  total           BIGINT,
  used            BIGINT,
  free            BIGINT,
  usage_percent   DOUBLE PRECISION
);
SELECT create_hypertable('resource_memory','time','host_ip',4, if_not_exists => TRUE);
```

3) Network（每行一块网卡）
```sql
CREATE TABLE resource_network (
  time        TIMESTAMPTZ NOT NULL,
  host_ip     INET        NOT NULL,
  interface   TEXT        NOT NULL,
  rx_bytes    BIGINT,
  tx_bytes    BIGINT,
  rx_packets  BIGINT,
  tx_packets  BIGINT,
  rx_errors   BIGINT,
  tx_errors   BIGINT,
  rx_rate     BIGINT,
  tx_rate     BIGINT
);
SELECT create_hypertable('resource_network','time','host_ip',4, if_not_exists => TRUE);
```

4) Disk（每行一个挂载/设备）
```sql
CREATE TABLE resource_disk (
  time            TIMESTAMPTZ NOT NULL,
  host_ip         INET        NOT NULL,
  device          TEXT        NOT NULL,
  mount_point     TEXT        NOT NULL,
  total           BIGINT,
  used            BIGINT,
  free            BIGINT,
  usage_percent   DOUBLE PRECISION
);
SELECT create_hypertable('resource_disk','time','host_ip',4, if_not_exists => TRUE);
```

5) GPU（每行一块 GPU）
```sql
CREATE TABLE resource_gpu (
  time           TIMESTAMPTZ NOT NULL,
  host_ip        INET        NOT NULL,
  gpu_index      INTEGER     NOT NULL,
  name           TEXT,
  compute_usage  DOUBLE PRECISION,
  mem_usage      DOUBLE PRECISION,
  mem_used       BIGINT,
  mem_total      BIGINT,
  temperature    DOUBLE PRECISION,
  power          DOUBLE PRECISION,
  gpu_allocated  INTEGER,    -- 可选：冗余存储节点级指标
  gpu_num        INTEGER     -- 可选
);
SELECT create_hypertable('resource_gpu','time','host_ip',4, if_not_exists => TRUE);
```

6) Component（每行一个组件实例）
```sql
CREATE TABLE component_resource (
  time        TIMESTAMPTZ NOT NULL,
  host_ip     INET        NOT NULL,
  instance_id TEXT        NOT NULL,
  uuid        TEXT        NOT NULL,
  idx         INTEGER     NOT NULL,
  name        TEXT,
  container_id TEXT,      -- 原 config.id
  state       TEXT,       -- PENDING/RUNNING/FAILED/STOPPED/SLEEPING
  cpu_load    DOUBLE PRECISION,
  mem_used    BIGINT,
  mem_limit   BIGINT,
  net_tx      BIGINT,
  net_rx      BIGINT
);
SELECT create_hypertable('component_resource','time','host_ip',4, if_not_exists => TRUE);
```

7) 可选：原始报文存档（便于审计/回放）
```sql
CREATE TABLE resource_raw (
  time     TIMESTAMPTZ NOT NULL,
  host_ip  INET        NOT NULL,
  payload  JSONB       NOT NULL
);
SELECT create_hypertable('resource_raw','time','host_ip',4, if_not_exists => TRUE);
```

### 四、索引建议
```sql
-- 常用时间倒序查询
CREATE INDEX ON resource_cpu      (host_ip, time DESC);
CREATE INDEX ON resource_memory   (host_ip, time DESC);
CREATE INDEX ON resource_network  (host_ip, interface, time DESC);
CREATE INDEX ON resource_disk     (host_ip, device, time DESC);
CREATE INDEX ON resource_gpu      (host_ip, gpu_index, time DESC);
CREATE INDEX ON component_resource(host_ip, instance_id, time DESC);
```

### 五、压缩与保留策略
```sql
-- 开启压缩
ALTER TABLE resource_cpu       SET (timescaledb.compress = true);
ALTER TABLE resource_memory    SET (timescaledb.compress = true);
ALTER TABLE resource_network   SET (timescaledb.compress = true);
ALTER TABLE resource_disk      SET (timescaledb.compress = true);
ALTER TABLE resource_gpu       SET (timescaledb.compress = true);
ALTER TABLE component_resource SET (timescaledb.compress = true);

-- 压缩策略：7天后压缩
SELECT add_compression_policy('resource_cpu',       INTERVAL '7 days');
SELECT add_compression_policy('resource_memory',    INTERVAL '7 days');
SELECT add_compression_policy('resource_network',   INTERVAL '7 days');
SELECT add_compression_policy('resource_disk',      INTERVAL '7 days');
SELECT add_compression_policy('resource_gpu',       INTERVAL '7 days');
SELECT add_compression_policy('component_resource', INTERVAL '7 days');

-- 归档/保留策略：90天后删除（按需调整）
SELECT add_retention_policy('resource_cpu',       INTERVAL '90 days');
SELECT add_retention_policy('resource_memory',    INTERVAL '90 days');
SELECT add_retention_policy('resource_network',   INTERVAL '90 days');
SELECT add_retention_policy('resource_disk',      INTERVAL '90 days');
SELECT add_retention_policy('resource_gpu',       INTERVAL '90 days');
SELECT add_retention_policy('component_resource', INTERVAL '90 days');
```

### 六、连续聚合（按需）
1) 主机级 CPU 1分钟均值
```sql
CREATE MATERIALIZED VIEW cpu_1m
WITH (timescaledb.continuous) AS
SELECT time_bucket('1 minute', time) AS bucket,
       host_ip,
       avg(usage_percent) AS usage_avg
FROM resource_cpu
GROUP BY bucket, host_ip;

SELECT add_continuous_aggregate_policy('cpu_1m',
  start_offset => INTERVAL '2 hours',
  end_offset   => INTERVAL '1 minute',
  schedule_interval => INTERVAL '1 minute');
```

2) 网卡速率 1分钟均值（可加 interface）
```sql
CREATE MATERIALIZED VIEW net_rate_1m
WITH (timescaledb.continuous) AS
SELECT time_bucket('1 minute', time) AS bucket,
       host_ip, interface,
       avg(rx_rate) AS rx_rate_avg,
       avg(tx_rate) AS tx_rate_avg
FROM resource_network
GROUP BY bucket, host_ip, interface;
```

其他（5m/15m、磁盘/GPU/组件）按需仿造。

### 七、写入建议
- /resource 接口收到一份报文时，生成一个 `now()` 时间戳，在一个事务内将其拆分写入各表（CPU/内存/网络[…]），保证同一批数据时间一致。
- 批量写入建议使用 `COPY` 或批量 `INSERT`；在线时可使用 libpqxx prepared statements。
- 原始 `payload` 可异步落盘至 `resource_raw` 以便追踪（可选）。

### 八、是否用 JSONB 存储？
- 时序查询/聚合频繁的指标（CPU/内存/网络/磁盘/GPU/组件）建议走列式设计（如上），性能与可索引性更好。
- 可附加一个 JSONB 原始表用于审计与兼容，将解析逻辑与存档解耦。

这样设计后，你可以高效按时间、主机、设备（网卡/磁盘/GPU/组件）维度做查询、聚合与告警，同时具备数据生命周期（压缩/保留）与汇总视图（连续聚合）。