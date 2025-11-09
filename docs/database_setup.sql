-- ============================================================================
-- 数据库初始化脚本（完整版）
-- 包含所有表结构：TimescaleDB 资源表、BMC 遥测表、告警规则表、告警事件表
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/database_setup.sql
--
-- 注意：
-- 1. 需要先安装 TimescaleDB 扩展：CREATE EXTENSION IF NOT EXISTS timescaledb;
-- 2. 本脚本使用事务，要么全部成功，要么全部回滚
-- ============================================================================

BEGIN;

-- ============================================================================
-- 第一部分：TimescaleDB 资源监控表（宽表方案）
-- ============================================================================

-- 1) 扩展（需要在事务外执行）
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


-- ============================================================================
-- 第二部分：BMC 遥测数据表（风扇和传感器）
-- ============================================================================

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


-- ============================================================================
-- 第三部分：告警规则表
-- ============================================================================

-- 创建告警规则表
CREATE TABLE IF NOT EXISTS alert_rules (
    -- 主键和标识
    id                  VARCHAR(100)    PRIMARY KEY,                    -- 系统生成的唯一标识符
    alert_name          VARCHAR(200)    NOT NULL UNIQUE,                -- 告警规则标识，用户自定义
    
    -- 告警规则配置
    expression          JSONB           NOT NULL,                       -- 告警表达式（JSON格式）
    for_duration        VARCHAR(20)     NOT NULL,                       -- 满足持续时间才产生告警，支持s/m/h单位
    severity            VARCHAR(50)     NOT NULL,                       -- 告警等级，用户自定义
    summary             TEXT            NOT NULL,                       -- 告警摘要，一小段话，用户自定义
    description         TEXT            NOT NULL DEFAULT '',            -- 告警详情，一大段话，用户自定义，支持占位符{{}}
    alert_type          VARCHAR(100)    NOT NULL,                       -- 告警类型，用户自定义
    enabled             BOOLEAN         NOT NULL DEFAULT TRUE,          -- 是否启用，默认为true
    
    -- 时间戳
    created_at          TIMESTAMP       NOT NULL DEFAULT NOW(),         -- 创建时间，系统自动生成
    updated_at          TIMESTAMP       NOT NULL DEFAULT NOW()          -- 更新时间，系统自动生成
);

-- 创建基础索引
CREATE INDEX IF NOT EXISTS idx_alert_rules_alert_name ON alert_rules(alert_name);
CREATE INDEX IF NOT EXISTS idx_alert_rules_alert_type ON alert_rules(alert_type);
CREATE INDEX IF NOT EXISTS idx_alert_rules_severity ON alert_rules(severity);
CREATE INDEX IF NOT EXISTS idx_alert_rules_enabled ON alert_rules(enabled);
CREATE INDEX IF NOT EXISTS idx_alert_rules_created_at ON alert_rules(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rules_updated_at ON alert_rules(updated_at DESC);

-- 创建JSONB字段的索引（使用B-tree索引）
CREATE INDEX IF NOT EXISTS idx_alert_rules_expression_stable ON alert_rules ((expression->>'stable'));
CREATE INDEX IF NOT EXISTS idx_alert_rules_expression_metric ON alert_rules ((expression->>'metric'));

-- 创建复合索引
CREATE INDEX IF NOT EXISTS idx_alert_rules_type_severity ON alert_rules(alert_type, severity);
CREATE INDEX IF NOT EXISTS idx_alert_rules_type_created ON alert_rules(alert_type, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rules_enabled_type ON alert_rules(enabled, alert_type);

-- 创建触发器函数，自动更新updated_at字段
CREATE OR REPLACE FUNCTION update_alert_rules_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- 创建触发器
CREATE TRIGGER trigger_update_alert_rules_updated_at
    BEFORE UPDATE ON alert_rules
    FOR EACH ROW
    EXECUTE FUNCTION update_alert_rules_updated_at();

-- 插入示例数据
INSERT INTO alert_rules (
    id, alert_name, expression, for_duration, severity, 
    summary, description, alert_type, enabled, created_at, updated_at
) VALUES (
    'rule_20250101_120000_123_5678',
    '磁盘使用率告警',
    '{
        "stable": "disk",
        "metric": "usage_percent",
        "conditions": [
            {
                "operator": ">",
                "threshold": 85
            }
        ],
        "tags": [
            {
                "mount_point": "/data"
            }
        ]
    }'::jsonb,
    '5s',
    '严重',
    '磁盘使用率过高',
    '磁盘{{mount_point}}使用率超过85%',
    '硬件资源',
    TRUE,
    NOW(),
    NOW()
) ON CONFLICT (id) DO NOTHING;

INSERT INTO alert_rules (
    id, alert_name, expression, for_duration, severity, 
    summary, description, alert_type, enabled, created_at, updated_at
) VALUES (
    'rule_20250101_120001_124_5679',
    'CPU使用率告警',
    '{
        "stable": "cpu",
        "metric": "usage_percent",
        "conditions": [
            {
                "operator": ">",
                "threshold": 80
            }
        ],
        "tags": []
    }'::jsonb,
    '30s',
    '一般',
    'CPU使用率过高',
    'CPU使用率超过80%',
    '硬件资源',
    TRUE,
    NOW(),
    NOW()
) ON CONFLICT (id) DO NOTHING;

INSERT INTO alert_rules (
    id, alert_name, expression, for_duration, severity, 
    summary, description, alert_type, enabled, created_at, updated_at
) VALUES (
    'rule_20250101_120002_125_5680',
    '节点离线告警',
    '{
        "stable": "alive",
        "metric": "alive",
        "conditions": [
            {
                "operator": "==",
                "threshold": 0
            }
        ],
        "tags": []
    }'::jsonb,
    '5s',
    '严重',
    '节点离线',
    '节点{{host_ip}}心跳超时',
    '系统告警',
    TRUE,
    NOW(),
    NOW()
) ON CONFLICT (id) DO NOTHING;


-- ============================================================================
-- 第四部分：告警事件表
-- ============================================================================

-- 创建告警表
CREATE TABLE IF NOT EXISTS alert (
    -- 主键和标识
    id                  VARCHAR(200)    PRIMARY KEY,                    -- 告警ID，系统生成的唯一标识符
    fingerprint         VARCHAR(500)    NOT NULL,                       -- 告警指纹，用于去重和识别
    
    -- 告警内容
    labels              JSONB           NOT NULL,                       -- 告警标签，JSON格式
    annotations         JSONB           NOT NULL,                       -- 告警注释，JSON格式
    
    -- 时间戳
    created_at          TIMESTAMP       NOT NULL DEFAULT NOW(),         -- 第一次匹配时间
    starts_at           TIMESTAMP,                                      -- 第一次正式触发告警时间
    updated_at          TIMESTAMP       NOT NULL DEFAULT NOW(),         -- 触发中持续匹配的更新时间
    ends_at             TIMESTAMP,                                      -- 告警已解决的时间
    
    -- 告警状态
    status              VARCHAR(20)     NOT NULL DEFAULT 'pending',     -- 告警状态：pending/firing/resolved
    
    -- 关联信息
    alert_rule_id       VARCHAR(100),                                  -- 关联的告警规则ID
    
    -- 约束
    CONSTRAINT chk_status CHECK (status IN ('pending', 'firing', 'resolved'))
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_alert_fingerprint ON alert(fingerprint);
CREATE INDEX IF NOT EXISTS idx_alert_status ON alert(status);
CREATE INDEX IF NOT EXISTS idx_alert_created_at ON alert(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_starts_at ON alert(starts_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_updated_at ON alert(updated_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_ends_at ON alert(ends_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rule_id ON alert(alert_rule_id);

-- 创建复合索引
CREATE INDEX IF NOT EXISTS idx_alert_status_created ON alert(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rule_status ON alert(alert_rule_id, status);

-- 创建JSONB字段的索引
CREATE INDEX IF NOT EXISTS idx_alert_labels_alert_name ON alert ((labels->>'alert_name'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_alert_type ON alert ((labels->>'alert_type'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_severity ON alert ((labels->>'severity'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_metric ON alert ((labels->>'metric'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_stable ON alert ((labels->>'stable'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_host_ip ON alert ((labels->>'host_ip'));

-- 创建触发器函数，自动更新updated_at字段
CREATE OR REPLACE FUNCTION update_alert_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- 创建触发器
CREATE TRIGGER trigger_update_alert_updated_at
    BEFORE UPDATE ON alert
    FOR EACH ROW
    EXECUTE FUNCTION update_alert_updated_at();

-- 添加表注释
COMMENT ON TABLE alert IS '告警表，存储系统中产生的所有告警';
COMMENT ON COLUMN alert.id IS '告警ID，系统生成的唯一标识符';
COMMENT ON COLUMN alert.fingerprint IS '告警指纹，用于去重和识别，格式为alert_name|key1=value1|key2=value2';
COMMENT ON COLUMN alert.labels IS '告警标签，JSON格式，包含节点IP、指标值、告警规则信息等';
COMMENT ON COLUMN alert.annotations IS '告警注释，JSON格式，包含summary和description';
COMMENT ON COLUMN alert.created_at IS '第一次匹配时间，可能还未满足持续时间条件';
COMMENT ON COLUMN alert.starts_at IS '第一次正式触发告警时间，满足持续时间条件';
COMMENT ON COLUMN alert.updated_at IS '触发中持续匹配的更新时间，每次更新';
COMMENT ON COLUMN alert.ends_at IS '告警已解决的时间';
COMMENT ON COLUMN alert.status IS '告警状态：pending(匹配但未满足持续时间)、firing(触发告警)、resolved(已解决)';
COMMENT ON COLUMN alert.alert_rule_id IS '关联的告警规则ID';

-- 插入示例数据（可选）
INSERT INTO alert (
    id, fingerprint, labels, annotations, created_at, starts_at, updated_at, 
    status, alert_rule_id
) VALUES (
    'alert_20251023_150000_001_1234',
    'CPU使用率告警|host_ip=192.168.1.100',
    '{
        "alert_name": "CPU使用率告警",
        "alert_type": "硬件资源",
        "host_ip": "192.168.1.100",
        "metric": "usage_percent",
        "severity": "严重",
        "stable": "cpu",
        "value": "85.500000"
    }'::jsonb,
    '{
        "summary": "CPU使用率过高",
        "description": "CPU使用率超过80%"
    }'::jsonb,
    NOW(),
    NOW(),
    NOW(),
    'firing',
    'rule_20250101_120000_123_5678'
) ON CONFLICT (id) DO NOTHING;

INSERT INTO alert (
    id, fingerprint, labels, annotations, created_at, starts_at, updated_at, 
    status, alert_rule_id
) VALUES (
    'alert_20251023_150001_002_5678',
    '磁盘使用率告警|host_ip=192.168.1.101|mount_point=/data',
    '{
        "alert_name": "磁盘使用率告警",
        "alert_type": "硬件资源",
        "host_ip": "192.168.1.101",
        "metric": "usage_percent",
        "severity": "严重",
        "stable": "disk",
        "mount_point": "/data",
        "value": "92.300000"
    }'::jsonb,
    '{
        "summary": "磁盘使用率过高",
        "description": "磁盘/data使用率超过85%"
    }'::jsonb,
    NOW(),
    NOW(),
    NOW(),
    'firing',
    'rule_20250101_120000_123_5678'
) ON CONFLICT (id) DO NOTHING;

INSERT INTO alert (
    id, fingerprint, labels, annotations, created_at, starts_at, updated_at, 
    status, alert_rule_id
) VALUES (
    'alert_20251023_150002_003_9012',
    '节点离线告警|host_ip=192.168.1.102',
    '{
        "alert_name": "节点离线告警",
        "alert_type": "可用性",
        "host_ip": "192.168.1.102",
        "metric": "alive",
        "severity": "严重",
        "stable": "alive",
        "value": "0.000000"
    }'::jsonb,
    '{
        "summary": "节点离线",
        "description": "节点192.168.1.102心跳超时"
    }'::jsonb,
    NOW(),
    NOW(),
    NOW(),
    'resolved',
    'rule_20250101_120002_125_5680'
) ON CONFLICT (id) DO NOTHING;

-- ============================================================================
-- 提交事务
-- ============================================================================

COMMIT;

-- ============================================================================
-- 初始化完成
-- ============================================================================
-- 已创建以下表：
-- 
-- TimescaleDB 时序表：
--   - resource_alive, resource_cpu, resource_memory, resource_network
--   - resource_disk, resource_gpu, component_resource, resource_raw
--   - bmc_fan, bmc_sensor
-- 
-- PostgreSQL 普通表：
--   - alert_rules (告警规则表)
--   - alert (告警事件表)
-- 
-- 所有表已创建索引、压缩策略和保留策略（如适用）
-- ============================================================================

