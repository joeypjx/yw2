-- 告警事件表设置 (V2 - 精简版本)
-- 支持告警事件的存储和查询

-- 创建告警事件表
CREATE TABLE IF NOT EXISTS alert_event (
    fingerprint     TEXT NOT NULL,              -- 告警指纹（唯一标识）
    labels          JSONB NOT NULL,             -- 标签集合
    status          TEXT NOT NULL,              -- 状态: inactive/pending/firing/resolved
    summary         TEXT NOT NULL DEFAULT '',   -- 告警摘要
    description     TEXT NOT NULL DEFAULT '',   -- 告警描述
    starts_at       TIMESTAMPTZ NOT NULL,       -- 开始时间
    ends_at         TIMESTAMPTZ,                -- 结束时间（可为空）
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(), -- 创建时间
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(), -- 更新时间
    PRIMARY KEY (fingerprint, starts_at)        -- 复合主键：同一告警的不同时间段
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_alert_event_status
ON alert_event (status, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_fingerprint
ON alert_event (fingerprint, starts_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_starts_at
ON alert_event (starts_at DESC);

-- 创建 GIN 索引用于 labels JSONB 查询
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_gin
ON alert_event USING GIN (labels);

-- 创建更新时间触发器
CREATE OR REPLACE FUNCTION update_alert_event_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = now();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_alert_event_updated_at
    BEFORE UPDATE ON alert_event
    FOR EACH ROW
    EXECUTE FUNCTION update_alert_event_updated_at();

-- 添加表注释
COMMENT ON TABLE alert_event IS '告警事件表V2，存储告警的触发和恢复记录';
COMMENT ON COLUMN alert_event.fingerprint IS '告警指纹，用于唯一标识一个告警实例';
COMMENT ON COLUMN alert_event.labels IS '标签集合（JSON格式），包含host_ip等维度信息';
COMMENT ON COLUMN alert_event.status IS '告警状态：inactive(未激活)/pending(待定)/firing(告警中)/resolved(已恢复)';
COMMENT ON COLUMN alert_event.summary IS '告警摘要，简要描述';
COMMENT ON COLUMN alert_event.description IS '告警详细描述，支持变量替换';
COMMENT ON COLUMN alert_event.starts_at IS '告警开始时间';
COMMENT ON COLUMN alert_event.ends_at IS '告警结束时间，firing状态时为NULL';
COMMENT ON COLUMN alert_event.created_at IS '记录创建时间';
COMMENT ON COLUMN alert_event.updated_at IS '记录最后更新时间';

-- 创建视图：活跃告警
CREATE OR REPLACE VIEW active_alert_events AS
SELECT
    fingerprint,
    labels,
    status,
    summary,
    description,
    starts_at,
    ends_at,
    created_at,
    updated_at
FROM alert_event
WHERE status IN ('pending', 'firing')
ORDER BY starts_at DESC;

-- 创建视图：最近的告警
CREATE OR REPLACE VIEW recent_alert_events AS
SELECT
    fingerprint,
    labels,
    status,
    summary,
    description,
    starts_at,
    ends_at,
    created_at,
    updated_at
FROM alert_event
WHERE starts_at > now() - INTERVAL '24 hours'
ORDER BY starts_at DESC;

-- 创建视图：按状态统计
CREATE OR REPLACE VIEW alert_event_statistics AS
SELECT
    status,
    COUNT(*) as event_count,
    COUNT(DISTINCT fingerprint) as unique_alerts,
    MIN(starts_at) as oldest_event,
    MAX(starts_at) as latest_event
FROM alert_event
WHERE starts_at > now() - INTERVAL '7 days'
GROUP BY status
ORDER BY status;

-- 示例数据（可选）
-- INSERT INTO alert_event (fingerprint, labels, status, summary, description, starts_at) VALUES
--     (
--         'cpu_high|host_ip=192.168.10.20',
--         '{"host_ip": "192.168.10.20", "host_type": "server"}'::jsonb,
--         'firing',
--         'CPU使用率超过80%',
--         '主机192.168.10.20的CPU使用率为85.6%，持续超过5分钟',
--         now() - INTERVAL '10 minutes'
--     );
