-- TimescaleDB 初始化与表结构（宽表方案）
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/timescaledb_setup.sql

BEGIN;

-- 1) 扩展
-- CREATE EXTENSION IF NOT EXISTS timescaledb;

-- 2) 资源表（宽表，分域）
-- 节点心跳 Alive（每次资源上报时打一条 1）
CREATE TABLE IF NOT EXISTS resource_alive (
  time     TIMESTAMPTZ NOT NULL,
  host_ip  INET        NOT NULL,
  alive    SMALLINT    NOT NULL
);
SELECT create_hypertable('resource_alive','time','host_ip',4, if_not_exists => TRUE);

-- CPU
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
SELECT create_hypertable('resource_cpu','time','host_ip',4, if_not_exists => TRUE);

-- Memory
CREATE TABLE IF NOT EXISTS resource_memory (
  time            TIMESTAMPTZ NOT NULL,
  host_ip         INET        NOT NULL,
  total           BIGINT,
  used            BIGINT,
  free            BIGINT,
  usage_percent   DOUBLE PRECISION
);
SELECT create_hypertable('resource_memory','time','host_ip',4, if_not_exists => TRUE);

-- Network（每行一块网卡）
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
SELECT create_hypertable('resource_network','time','host_ip',4, if_not_exists => TRUE);

-- Disk（每行一个挂载/设备）
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
SELECT create_hypertable('resource_disk','time','host_ip',4, if_not_exists => TRUE);

-- GPU（每行一块 GPU）
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
  gpu_allocated  INTEGER,
  gpu_num        INTEGER
);
SELECT create_hypertable('resource_gpu','time','host_ip',4, if_not_exists => TRUE);

-- Component（每行一个组件实例）
CREATE TABLE IF NOT EXISTS component_resource (
  time        TIMESTAMPTZ NOT NULL,
  host_ip     INET        NOT NULL,
  instance_id TEXT        NOT NULL,
  uuid        TEXT        NOT NULL,
  idx         INTEGER     NOT NULL,
  name        TEXT,
  container_id TEXT,
  state       TEXT,
  cpu_load    DOUBLE PRECISION,
  mem_used    BIGINT,
  mem_limit   BIGINT,
  net_tx      BIGINT,
  net_rx      BIGINT,
  net_rx_rate DOUBLE PRECISION,
  net_tx_rate DOUBLE PRECISION
);
SELECT create_hypertable('component_resource','time','host_ip',4, if_not_exists => TRUE);

-- 原始报文（可选）
CREATE TABLE IF NOT EXISTS resource_raw (
  time     TIMESTAMPTZ NOT NULL,
  host_ip  INET        NOT NULL,
  payload  JSONB       NOT NULL
);
SELECT create_hypertable('resource_raw','time','host_ip',4, if_not_exists => TRUE);

-- 3) 索引
CREATE INDEX IF NOT EXISTS idx_cpu_host_time          ON resource_cpu      (host_ip, time DESC);
CREATE INDEX IF NOT EXISTS idx_mem_host_time          ON resource_memory   (host_ip, time DESC);
CREATE INDEX IF NOT EXISTS idx_net_host_iface_time    ON resource_network  (host_ip, interface, time DESC);
CREATE INDEX IF NOT EXISTS idx_disk_host_dev_time     ON resource_disk     (host_ip, device, time DESC);
CREATE INDEX IF NOT EXISTS idx_gpu_host_idx_time      ON resource_gpu      (host_ip, gpu_index, time DESC);
CREATE INDEX IF NOT EXISTS idx_comp_host_inst_time    ON component_resource(host_ip, instance_id, time DESC);
CREATE INDEX IF NOT EXISTS idx_alive_host_time        ON resource_alive     (host_ip, time DESC);

-- 4) 压缩与保留策略
-- 显式设置压缩策略，指定 orderby/segmentby 以消除默认推断告警
ALTER TABLE resource_cpu       SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);
ALTER TABLE resource_memory    SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);
ALTER TABLE resource_network   SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'interface'
);
ALTER TABLE resource_disk      SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'device'
);
ALTER TABLE resource_gpu       SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'gpu_index'
);
ALTER TABLE component_resource SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'instance_id'
);
ALTER TABLE resource_alive SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

SELECT add_compression_policy('resource_cpu',       INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_memory',    INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_network',   INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_disk',      INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_gpu',       INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('component_resource', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_alive',     INTERVAL '7 days', if_not_exists => TRUE);

SELECT add_retention_policy('resource_cpu',       INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_memory',    INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_network',   INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_disk',      INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_gpu',       INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('component_resource', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_alive',     INTERVAL '90 days', if_not_exists => TRUE);

COMMIT;


