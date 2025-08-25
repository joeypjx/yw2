-- 告警事件表基本设置（不依赖 TimescaleDB）
-- 用于存储告警系统的所有事件记录

-- 创建告警事件表
CREATE TABLE IF NOT EXISTS alert_event (
    time        TIMESTAMPTZ NOT NULL,    -- 事件时间
    fingerprint TEXT NOT NULL,           -- 告警指纹（唯一标识）
    rule_id     TEXT NOT NULL,           -- 规则ID
    action      TEXT NOT NULL,           -- 动作类型（pending, firing, resolved等）
    status      TEXT NOT NULL,           -- 告警状态
    severity    TEXT NOT NULL,           -- 严重级别（info, warn, critical）
    labels      JSONB,                   -- 标签集合
    title       TEXT,                    -- 告警标题
    description TEXT,                    -- 告警描述
    value       DOUBLE PRECISION,        -- 数值（如CPU使用率）
    unit        TEXT,                    -- 单位（如百分比、MB等）
    context     JSONB                    -- 上下文信息（如主机信息、阈值等）
);

-- 创建时间索引（用于时间范围查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_time 
ON alert_event (time DESC);

-- 创建指纹索引（用于特定告警查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_fingerprint 
ON alert_event (fingerprint, time DESC);

-- 创建规则ID索引（用于按规则查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_rule_id 
ON alert_event (rule_id, time DESC);

-- 创建状态索引（用于按状态查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_status 
ON alert_event (status, time DESC);

-- 创建严重级别索引（用于按级别查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_severity 
ON alert_event (severity, time DESC);

-- 创建复合索引（用于复杂查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_composite 
ON alert_event (status, severity, time DESC);

-- 添加表注释
COMMENT ON TABLE alert_event IS '告警事件表，存储所有告警状态变化和事件记录';
COMMENT ON COLUMN alert_event.time IS '事件发生时间';
COMMENT ON COLUMN alert_event.fingerprint IS '告警指纹，用于唯一标识告警实例';
COMMENT ON COLUMN alert_event.rule_id IS '触发此事件的告警规则ID';
COMMENT ON COLUMN alert_event.action IS '事件动作类型：pending(待处理)、firing(触发中)、resolved(已解决)';
COMMENT ON COLUMN alert_event.status IS '告警状态：inactive(非活动)、pending(待处理)、firing(触发中)、resolved(已解决)';
COMMENT ON COLUMN alert_event.severity IS '告警严重级别：info(信息)、warn(警告)、critical(严重)';
COMMENT ON COLUMN alert_event.labels IS '标签集合，用于告警分组和过滤';
COMMENT ON COLUMN alert_event.title IS '告警标题';
COMMENT ON COLUMN alert_event.description IS '告警详细描述';
COMMENT ON COLUMN alert_event.value IS '告警相关的数值（如CPU使用率、内存使用量等）';
COMMENT ON COLUMN alert_event.unit IS '数值的单位（如百分比、MB、GB等）';
COMMENT ON COLUMN alert_event.context IS '告警上下文信息，包含触发条件、阈值等详细信息';

-- 创建一些有用的视图

-- 活跃告警视图（当前处于firing状态的告警）
CREATE OR REPLACE VIEW active_alerts AS
SELECT DISTINCT ON (fingerprint)
    fingerprint,
    rule_id,
    title,
    description,
    severity,
    labels,
    value,
    unit,
    context,
    time as last_update
FROM alert_event
WHERE status = 'firing'
ORDER BY fingerprint, time DESC;

-- 告警统计视图
CREATE OR REPLACE VIEW alert_statistics AS
SELECT 
    status,
    severity,
    COUNT(*) as count,
    MIN(time) as first_occurrence,
    MAX(time) as last_occurrence
FROM alert_event
WHERE time > now() - INTERVAL '24 hours'
GROUP BY status, severity
ORDER BY status, severity;

-- 规则告警统计视图
CREATE OR REPLACE VIEW rule_alert_stats AS
SELECT 
    rule_id,
    COUNT(*) as total_events,
    COUNT(DISTINCT fingerprint) as unique_alerts,
    COUNT(CASE WHEN status = 'firing' THEN 1 END) as active_alerts,
    COUNT(CASE WHEN status = 'resolved' THEN 1 END) as resolved_alerts,
    MIN(time) as first_occurrence,
    MAX(time) as last_occurrence
FROM alert_event
WHERE time > now() - INTERVAL '7 days'
GROUP BY rule_id
ORDER BY total_events DESC;
