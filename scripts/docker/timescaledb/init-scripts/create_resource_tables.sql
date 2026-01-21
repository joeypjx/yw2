-- ============================================================================
-- Monitor 模块数据库表创建脚本
-- 
-- 根据 docs/monitor/database_schema.md 文档生成
-- 包含表结构创建、索引配置、TimescaleDB hypertable 创建、压缩和保留策略
-- 
-- 前置条件：
-- 1. 需要先安装 TimescaleDB 扩展：CREATE EXTENSION IF NOT EXISTS timescaledb;
-- 2. 确保数据库已启用 TimescaleDB
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/monitor/create_tables.sql
-- ============================================================================

CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

-- 1. resource_alive - 节点心跳表
-- 用于记录节点的存活状态，每次资源上报时插入一条记录
CREATE TABLE IF NOT EXISTS resource_alive (
  time     TIMESTAMPTZ NOT NULL,
  host_ip  INET        NOT NULL,
  alive    SMALLINT    NOT NULL
);

-- 2. resource_cpu - CPU 资源表
-- 用于记录节点的 CPU 资源使用情况
CREATE TABLE IF NOT EXISTS resource_cpu (
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

-- 3. resource_memory - 内存资源表
-- 用于记录节点的内存资源使用情况
CREATE TABLE IF NOT EXISTS resource_memory (
  time            TIMESTAMPTZ NOT NULL,
  host_ip         INET        NOT NULL,
  total           BIGINT,
  used            BIGINT,
  free            BIGINT,
  usage_percent   DOUBLE PRECISION
);

-- 4. resource_network - 网络资源表
-- 用于记录节点的网络接口资源使用情况，每行代表一个网络接口
CREATE TABLE IF NOT EXISTS resource_network (
  time        TIMESTAMPTZ NOT NULL,
  host_ip     INET        NOT NULL,
  interface   TEXT        NOT NULL,
  rx_bytes    BIGINT,
  tx_bytes    BIGINT,
  rx_packets  BIGINT,
  tx_packets  BIGINT,
  rx_errors   BIGINT,
  tx_errors   BIGINT,
  rx_rate     DOUBLE PRECISION,
  tx_rate     DOUBLE PRECISION,
  rx_drop_rate DOUBLE PRECISION,
  tx_drop_rate DOUBLE PRECISION,
  state       INTEGER
);

-- 5. resource_disk - 磁盘资源表
-- 用于记录节点的磁盘资源使用情况，每行代表一个磁盘设备或挂载点
CREATE TABLE IF NOT EXISTS resource_disk (
  time            TIMESTAMPTZ NOT NULL,
  host_ip         INET        NOT NULL,
  device          TEXT        NOT NULL,
  mount_point     TEXT        NOT NULL,
  total           BIGINT,
  used            BIGINT,
  free            BIGINT,
  usage_percent   DOUBLE PRECISION
);

-- 6. resource_gpu - GPU 资源表
-- 用于记录节点的 GPU 资源使用情况，每行代表一块 GPU
CREATE TABLE IF NOT EXISTS resource_gpu (
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
  free           INTEGER,
  gpu_allocated  INTEGER,
  gpu_num        INTEGER
);

-- 7. resource_component - 组件资源表
-- 用于记录节点上运行的组件（容器）的资源使用情况，每行代表一个组件实例
CREATE TABLE IF NOT EXISTS resource_component (
  time        TIMESTAMPTZ NOT NULL,
  host_ip     INET        NOT NULL,
  instance_id TEXT        NOT NULL,
  uuid        TEXT        NOT NULL,
  idx         INTEGER     NOT NULL,
  name        TEXT,
  container_id TEXT,
  state       TEXT,
  type        TEXT,
  cpu_load    DOUBLE PRECISION,
  mem_used    BIGINT,
  mem_limit   BIGINT,
  mem_usage   DOUBLE PRECISION,
  net_tx      BIGINT,
  net_rx      BIGINT,
  net_rx_rate DOUBLE PRECISION,
  net_tx_rate DOUBLE PRECISION
);

-- 8. resource_raw - 原始报文表（可选）
-- 用于存储原始的资源上报报文，便于后续分析和调试
CREATE TABLE IF NOT EXISTS resource_raw (
  time     TIMESTAMPTZ NOT NULL,
  host_ip  INET        NOT NULL,
  payload  JSONB       NOT NULL
);

-- ============================================================================
-- 索引创建
-- ============================================================================

-- resource_alive 表索引
-- 用于时间范围查询和按节点过滤（TimescaleDB 时序表常用查询模式）
CREATE INDEX IF NOT EXISTS idx_resource_alive_time_host_ip ON resource_alive(time DESC, host_ip);

-- resource_cpu 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_cpu_time_host_ip ON resource_cpu(time DESC, host_ip);

-- resource_memory 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_memory_time_host_ip ON resource_memory(time DESC, host_ip);

-- resource_network 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_network_time_host_ip ON resource_network(time DESC, host_ip);

-- 用于按节点和接口查询（告警规则常用查询模式）
CREATE INDEX IF NOT EXISTS idx_resource_network_time_host_ip_interface ON resource_network(time DESC, host_ip, interface);

-- resource_disk 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_disk_time_host_ip ON resource_disk(time DESC, host_ip);

-- 用于按节点、设备和挂载点查询（告警规则常用查询模式）
CREATE INDEX IF NOT EXISTS idx_resource_disk_time_host_ip_device_mount ON resource_disk(time DESC, host_ip, device, mount_point);

-- resource_gpu 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_gpu_time_host_ip ON resource_gpu(time DESC, host_ip);

-- 用于按节点和GPU索引查询（告警规则常用查询模式）
CREATE INDEX IF NOT EXISTS idx_resource_gpu_time_host_ip_gpu_index ON resource_gpu(time DESC, host_ip, gpu_index);

-- resource_component 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_component_time_host_ip ON resource_component(time DESC, host_ip);

-- 用于按节点和实例ID查询
CREATE INDEX IF NOT EXISTS idx_resource_component_time_host_ip_instance_id ON resource_component(time DESC, host_ip, instance_id);

-- resource_raw 表索引
-- 用于时间范围查询和按节点过滤
CREATE INDEX IF NOT EXISTS idx_resource_raw_time_host_ip ON resource_raw(time DESC, host_ip);

-- 用于 JSONB 查询（如果需要查询 payload 内容）
CREATE INDEX IF NOT EXISTS idx_resource_raw_payload_gin ON resource_raw USING GIN (payload);

-- ============================================================================
-- TimescaleDB Hypertable 创建
-- ============================================================================

-- 将表转换为 TimescaleDB hypertable（时序表）
-- 使用 time 作为分区键，host_ip 作为分段键（用于分布式场景）
SELECT create_hypertable('resource_alive', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_cpu', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_memory', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_network', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_disk', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_gpu', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_component', 'time', 'host_ip', 4, if_not_exists => TRUE);
SELECT create_hypertable('resource_raw', 'time', 'host_ip', 4, if_not_exists => TRUE);

-- ============================================================================
-- 压缩配置
-- ============================================================================

-- 启用压缩并设置压缩参数
-- compress_orderby: 指定压缩时的排序顺序，通常与查询的 ORDER BY 一致
-- compress_segmentby: 指定分段键，相同值的行会被分组在一起（提高压缩比）

-- resource_alive 表压缩配置
ALTER TABLE resource_alive SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

-- resource_cpu 表压缩配置
ALTER TABLE resource_cpu SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

-- resource_memory 表压缩配置
ALTER TABLE resource_memory SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

-- resource_network 表压缩配置（有标签列 interface）
ALTER TABLE resource_network SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'interface'
);

-- resource_disk 表压缩配置（有标签列 device 和 mount_point）
ALTER TABLE resource_disk SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'device'
);

-- resource_gpu 表压缩配置（有标签列 gpu_index）
ALTER TABLE resource_gpu SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'gpu_index'
);

-- resource_component 表压缩配置（有标签列 instance_id）
ALTER TABLE resource_component SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'instance_id'
);

-- resource_raw 表压缩配置
ALTER TABLE resource_raw SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

-- ============================================================================
-- 压缩策略
-- ============================================================================

-- 添加压缩策略：数据超过 7 天后自动压缩
-- 7 天内的数据保持未压缩状态，便于快速查询和写入
-- 7 天后的数据自动压缩，节省存储空间（压缩比通常可达 90% 以上）
SELECT add_compression_policy('resource_alive', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_cpu', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_memory', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_network', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_disk', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_gpu', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_component', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_raw', INTERVAL '7 days', if_not_exists => TRUE);

-- ============================================================================
-- 保留策略
-- ============================================================================

-- 添加保留策略：数据超过 90 天后自动删除
-- 保留 90 天的历史数据，用于分析和趋势预测
-- 超过 90 天的数据自动删除，释放存储空间
SELECT add_retention_policy('resource_alive', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_cpu', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_memory', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_network', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_disk', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_gpu', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_component', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_raw', INTERVAL '90 days', if_not_exists => TRUE);

