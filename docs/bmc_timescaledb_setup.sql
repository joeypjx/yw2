-- BMC telemetry schema (fans and sensors)
-- Field names align with C structs in src/bmc/bmc_model.h

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


