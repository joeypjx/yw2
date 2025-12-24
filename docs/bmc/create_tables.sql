-- ============================================================================
-- BMC 模块数据库表创建脚本
-- 
-- 根据 docs/bmc/database_schema.md 文档生成
-- 仅包含表结构创建，不包含索引、压缩策略等配置
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

