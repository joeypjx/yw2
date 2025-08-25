-- TimescaleDB 初始化与表结构（宽表方案）
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/timescaledb_setup.sql

BEGIN;

-- 1) 扩展
-- CREATE EXTENSION IF NOT EXISTS timescaledb;

-- 2) 资源表（宽表，分域）
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
  rx_rate     BIGINT,
  tx_rate     BIGINT
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
  net_rx      BIGINT
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

SELECT add_compression_policy('resource_cpu',       INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_memory',    INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_network',   INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_disk',      INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_gpu',       INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('component_resource', INTERVAL '7 days', if_not_exists => TRUE);

SELECT add_retention_policy('resource_cpu',       INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_memory',    INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_network',   INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_disk',      INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_gpu',       INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('component_resource', INTERVAL '90 days', if_not_exists => TRUE);


-- 5) BMC telemetry schema (fans and sensors)
-- Field names align with C structs in include/yw/bmc_model.h

-- Fans (UdpFanInfo + context from UdpInfo)
CREATE TABLE IF NOT EXISTS bmc_fan (
    "time" TIMESTAMPTZ NOT NULL, -- UdpInfo.timestamp
    boxid       INTEGER      NOT NULL, -- UdpInfo.boxid
    fanseq      SMALLINT,              -- UdpFanInfo.fanseq
    fanmode     SMALLINT,              -- UdpFanInfo.fanmode (原始字节，不拆高低位)
    fanspeed    INTEGER                -- UdpFanInfo.fanspeed
);

SELECT create_hypertable('bmc_fan', 'time', if_not_exists => TRUE, migrate_data => TRUE);

ALTER TABLE bmc_fan SET (
    timescaledb.compress,
    timescaledb.compress_segmentby = 'boxid',
    timescaledb.compress_orderby = 'time DESC'
);

CREATE INDEX IF NOT EXISTS idx_bmc_fan_box_time ON bmc_fan(boxid, "time" DESC);
CREATE INDEX IF NOT EXISTS idx_bmc_fan_fanseq_time ON bmc_fan(fanseq, "time" DESC);

SELECT add_compression_policy('bmc_fan', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_retention_policy('bmc_fan',   INTERVAL '30 days', if_not_exists => TRUE);


-- Sensors (UdpSensorInfo + minimal context from UdpInfo)
CREATE TABLE IF NOT EXISTS bmc_sensor (
    "time"   TIMESTAMPTZ NOT NULL, -- UdpInfo.timestamp
    host_ip       INET         NOT NULL, -- 由 boxid 与 ipmbaddr 映射
    sensorseq     SMALLINT,              -- UdpSensorInfo.sensorseq
    sensortype    SMALLINT,              -- UdpSensorInfo.sensortype
    sensorname    TEXT,                  -- UdpSensorInfo.sensorname[6] 转字符串
    sensorvalue_L SMALLINT,              -- UdpSensorInfo.sensorvalue_L
    sensorvalue_H SMALLINT,              -- UdpSensorInfo.sensorvalue_H
    sensoralmtype SMALLINT               -- UdpSensorInfo.sensoralmtype
);

SELECT create_hypertable('bmc_sensor', 'time', if_not_exists => TRUE, migrate_data => TRUE);

ALTER TABLE bmc_sensor SET (
    timescaledb.compress,
    timescaledb.compress_segmentby = 'host_ip',
    timescaledb.compress_orderby = 'time DESC'
);

CREATE INDEX IF NOT EXISTS idx_bmc_sensor_host_time ON bmc_sensor(host_ip, "time" DESC);
CREATE INDEX IF NOT EXISTS idx_bmc_sensor_type_time ON bmc_sensor(sensortype, "time" DESC);
CREATE INDEX IF NOT EXISTS idx_bmc_sensor_name_time ON bmc_sensor(sensorname, "time" DESC);

SELECT add_compression_policy('bmc_sensor', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_retention_policy('bmc_sensor',   INTERVAL '90 days', if_not_exists => TRUE);

-- 6) Alerting schema（精简版：仅规则、活跃告警与事件）

-- 规则表
CREATE TABLE IF NOT EXISTS alert_rule (
  id           TEXT PRIMARY KEY,
  name         TEXT,
  expression   TEXT NOT NULL,
  window       TEXT NOT NULL,
  eval_every   TEXT NOT NULL,
  severity     TEXT NOT NULL,     -- info/warn/critical
  selector     JSONB,             -- 标签选择器
  for_times    INTEGER DEFAULT 1, -- 连续命中次数
  enabled      BOOLEAN DEFAULT TRUE,
  updated_at   TIMESTAMPTZ DEFAULT now()
);

-- 活跃告警（Hypertable 便于时间序列维护与查询）
CREATE TABLE IF NOT EXISTS alert_active (
  time            TIMESTAMPTZ NOT NULL,
  fingerprint     TEXT PRIMARY KEY,
  rule_id         TEXT NOT NULL,
  status          TEXT NOT NULL,     -- inactive/pending/firing/resolved
  severity        TEXT NOT NULL,
  labels          JSONB,
  first_firing_ms BIGINT,
  last_eval_ms    BIGINT,
  last_change_ms  BIGINT,
  notify_cool_ms  BIGINT,
  occurrences     BIGINT,
  acked           BOOLEAN DEFAULT FALSE,
  acked_by        TEXT,
  acked_at_ms     BIGINT
);
SELECT create_hypertable('alert_active','time', if_not_exists => TRUE);
CREATE INDEX IF NOT EXISTS idx_alert_active_status_time ON alert_active(status, time DESC);
CREATE INDEX IF NOT EXISTS idx_alert_active_rule_time   ON alert_active(rule_id, time DESC);

-- 告警事件流水（Hypertable）
CREATE TABLE IF NOT EXISTS alert_event (
  time        TIMESTAMPTZ NOT NULL,
  fingerprint TEXT NOT NULL,
  rule_id     TEXT NOT NULL,
  action      TEXT NOT NULL,  -- firing/resolved/ack/notified/escalated
  status      TEXT NOT NULL,
  severity    TEXT NOT NULL,
  labels      JSONB,
  title       TEXT,
  description TEXT,
  value       DOUBLE PRECISION,
  unit        TEXT,
  context     JSONB
);
SELECT create_hypertable('alert_event','time', if_not_exists => TRUE);
CREATE INDEX IF NOT EXISTS idx_alert_event_rule_time ON alert_event(rule_id, time DESC);
CREATE INDEX IF NOT EXISTS idx_alert_event_fp_time   ON alert_event(fingerprint, time DESC);

COMMIT;


