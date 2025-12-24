-- ============================================================================
-- Monitor 模块数据库表创建脚本
-- 
-- 根据 docs/monitor/database_schema.md 文档生成
-- 仅包含表结构创建，不包含索引、压缩策略等配置
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/monitor/create_tables.sql
-- ============================================================================

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

