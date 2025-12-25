-- ============================================================================
-- BMC 模块数据库表创建脚本
-- 
-- 根据 docs/bmc/database_schema.md 文档生成
-- 包含表结构创建、索引配置、TimescaleDB hypertable 创建、压缩和保留策略
-- 
-- 前置条件：
-- 1. 需要先安装 TimescaleDB 扩展：CREATE EXTENSION IF NOT EXISTS timescaledb;
-- 2. 确保数据库已启用 TimescaleDB
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/bmc/create_tables.sql
-- ============================================================================

-- 1. bmc_fan - 风扇表
-- 用于记录机箱风扇的运行状态和速度信息
CREATE TABLE IF NOT EXISTS bmc_fan (
    "time"   TIMESTAMPTZ NOT NULL,
    boxid    INTEGER     NOT NULL,
    fanseq   SMALLINT,
    fanmode  SMALLINT,
    fanspeed INTEGER
);

-- 2. bmc_sensor - 传感器表
-- 用于记录负载槽板卡上的传感器数据，包括温度、电压、电流等传感器信息
CREATE TABLE IF NOT EXISTS bmc_sensor (
    "time"        TIMESTAMPTZ NOT NULL,
    host_ip       INET        NOT NULL,
    sensorseq     SMALLINT,
    sensortype    SMALLINT,
    sensorname    TEXT,
    sensorvalue_L SMALLINT,
    sensorvalue_H SMALLINT,
    sensoralmtype SMALLINT
);

-- ============================================================================
-- 索引创建
-- ============================================================================

-- bmc_fan 表索引
-- 用于时间范围查询和按机箱过滤（TimescaleDB 时序表常用查询模式）
CREATE INDEX IF NOT EXISTS idx_bmc_fan_time_boxid ON bmc_fan("time" DESC, boxid);

-- 用于按机箱和风扇序号查询
CREATE INDEX IF NOT EXISTS idx_bmc_fan_boxid_fanseq ON bmc_fan(boxid, fanseq);

-- bmc_sensor 表索引
-- 用于时间范围查询和按节点过滤（TimescaleDB 时序表常用查询模式）
CREATE INDEX IF NOT EXISTS idx_bmc_sensor_time_host_ip ON bmc_sensor("time" DESC, host_ip);

-- 用于按节点和传感器序号查询
CREATE INDEX IF NOT EXISTS idx_bmc_sensor_host_ip_sensorseq ON bmc_sensor(host_ip, sensorseq);

-- ============================================================================
-- TimescaleDB Hypertable 创建
-- ============================================================================

-- 将表转换为 TimescaleDB hypertable（时序表）
-- 使用 time 作为分区键
SELECT create_hypertable('bmc_fan', 'time', if_not_exists => TRUE, migrate_data => TRUE);
SELECT create_hypertable('bmc_sensor', 'time', if_not_exists => TRUE, migrate_data => TRUE);

-- ============================================================================
-- 压缩配置
-- ============================================================================

-- 启用压缩并设置压缩参数
-- compress_orderby: 指定压缩时的排序顺序，通常与查询的 ORDER BY 一致
-- compress_segmentby: 指定分段键，相同值的行会被分组在一起（提高压缩比）

-- bmc_fan 表压缩配置（按 boxid 分段）
ALTER TABLE bmc_fan SET (
  timescaledb.compress = true,
  timescaledb.compress_segmentby = 'boxid',
  timescaledb.compress_orderby = 'time DESC'
);

-- bmc_sensor 表压缩配置（按 host_ip 分段）
ALTER TABLE bmc_sensor SET (
  timescaledb.compress = true,
  timescaledb.compress_segmentby = 'host_ip',
  timescaledb.compress_orderby = 'time DESC'
);

-- ============================================================================
-- 压缩策略
-- ============================================================================

-- 添加压缩策略：数据超过 7 天后自动压缩
-- 7 天内的数据保持未压缩状态，便于快速查询和写入
-- 7 天后的数据自动压缩，节省存储空间（压缩比通常可达 90% 以上）
SELECT add_compression_policy('bmc_fan', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('bmc_sensor', INTERVAL '7 days', if_not_exists => TRUE);

-- ============================================================================
-- 保留策略
-- ============================================================================

-- 添加保留策略：数据超过保留期后自动删除
-- bmc_fan: 30 天后删除（风扇数据变化频率高，保留期较短）
-- bmc_sensor: 90 天后删除（传感器数据可能用于长期分析）
SELECT add_retention_policy('bmc_fan', INTERVAL '30 days', if_not_exists => TRUE);
SELECT add_retention_policy('bmc_sensor', INTERVAL '90 days', if_not_exists => TRUE);

