-- 告警系统完整设置
-- 包含告警规则表和告警事件表的所有功能

-- ========================================
-- 1. 告警规则表设置
-- ========================================

-- 创建告警规则表
CREATE TABLE IF NOT EXISTS alert_rule (
    id          TEXT PRIMARY KEY,           -- 规则ID（唯一标识）
    name        TEXT NOT NULL,              -- 规则名称
    expression  TEXT NOT NULL,              -- 告警表达式（DSL格式）
    time_window TEXT NOT NULL,              -- 时间窗口（如：5m, 1h）
    eval_every  TEXT NOT NULL,              -- 评估频率（如：30s, 1m）
    severity    TEXT NOT NULL,              -- 严重级别（info, warn, critical）
    selector    JSONB,                      -- 标签选择器（用于过滤目标）
    for_times   INTEGER NOT NULL DEFAULT 1, -- 连续匹配次数（防抖动）
    enabled     BOOLEAN NOT NULL DEFAULT true, -- 是否启用
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(), -- 创建时间
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()  -- 更新时间
);

-- 创建规则表索引
CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled 
ON alert_rule (enabled, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_rule_severity 
ON alert_rule (severity, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_rule_name 
ON alert_rule (name);

-- 创建规则表更新时间触发器
CREATE OR REPLACE FUNCTION update_alert_rule_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = now();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_alert_rule_updated_at
    BEFORE UPDATE ON alert_rule
    FOR EACH ROW
    EXECUTE FUNCTION update_alert_rule_updated_at();

-- ========================================
-- 2. 告警事件表设置
-- ========================================

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

-- 创建事件表索引
CREATE INDEX IF NOT EXISTS idx_alert_event_time 
ON alert_event (time DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_fingerprint 
ON alert_event (fingerprint, time DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_rule_id 
ON alert_event (rule_id, time DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_status 
ON alert_event (status, time DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_severity 
ON alert_event (severity, time DESC);

CREATE INDEX IF NOT EXISTS idx_alert_event_composite 
ON alert_event (status, severity, time DESC);

-- ========================================
-- 3. TimescaleDB 功能（如果可用）
-- ========================================

DO $$
BEGIN
    IF EXISTS (
        SELECT 1 FROM pg_extension WHERE extname = 'timescaledb'
    ) THEN
        -- 将事件表转换为 TimescaleDB 超表
        PERFORM create_hypertable('alert_event', 'time', 
            if_not_exists => TRUE,
            chunk_time_interval => INTERVAL '1 day'
        );
        
        -- 启用压缩
        EXECUTE 'ALTER TABLE alert_event SET (
            timescaledb.compress,
            timescaledb.compress_segmentby = ''fingerprint, rule_id, status, severity''
        )';
        
        -- 添加压缩策略（7天后开始压缩，每个chunk压缩为1天）
        IF EXISTS (
            SELECT 1 FROM pg_proc WHERE proname = 'add_compression_policy'
        ) THEN
            PERFORM add_compression_policy('alert_event', 
                INTERVAL '7 days',
                if_not_exists => TRUE
            );
        END IF;
        
        -- 添加保留策略（保留90天的数据）
        IF EXISTS (
            SELECT 1 FROM pg_proc WHERE proname = 'add_retention_policy'
        ) THEN
            PERFORM add_retention_policy('alert_event', 
                INTERVAL '90 days',
                if_not_exists => TRUE
            );
        END IF;
        
        -- 创建连续聚合视图
        EXECUTE 'CREATE MATERIALIZED VIEW IF NOT EXISTS alert_event_hourly_stats AS
        SELECT 
            time_bucket(''1 hour'', time) AS bucket,
            status,
            severity,
            COUNT(*) as event_count,
            COUNT(DISTINCT fingerprint) as unique_alerts
        FROM alert_event
        GROUP BY bucket, status, severity
        ORDER BY bucket DESC';
        
        -- 为连续聚合视图创建索引
        EXECUTE 'CREATE INDEX IF NOT EXISTS idx_alert_event_hourly_stats_bucket 
        ON alert_event_hourly_stats (bucket DESC)';
    END IF;
END $$;

-- ========================================
-- 4. 连续聚合策略设置（如果 TimescaleDB 可用）
-- ========================================

DO $$
BEGIN
    IF EXISTS (
        SELECT 1 FROM pg_extension WHERE extname = 'timescaledb'
    ) AND EXISTS (
        SELECT 1 FROM pg_proc WHERE proname = 'add_continuous_aggregate_policy'
    ) AND EXISTS (
        SELECT 1 FROM pg_class WHERE relname = 'alert_event_hourly_stats'
    ) THEN
        -- 设置连续聚合视图的刷新策略
        PERFORM add_continuous_aggregate_policy('alert_event_hourly_stats',
            start_offset => INTERVAL '3 hours',
            end_offset => INTERVAL '1 hour',
            schedule_interval => INTERVAL '1 hour'
        );
    END IF;
END $$;

-- ========================================
-- 5. 表注释
-- ========================================

-- 规则表注释
COMMENT ON TABLE alert_rule IS '告警规则表，存储告警系统的规则配置';
COMMENT ON COLUMN alert_rule.id IS '规则唯一标识符';
COMMENT ON COLUMN alert_rule.name IS '规则名称，用于描述告警内容';
COMMENT ON COLUMN alert_rule.expression IS '告警表达式，使用DSL格式（如：cpu.usage_percent.avg > 80）';
COMMENT ON COLUMN alert_rule.time_window IS '时间窗口，用于聚合计算（如：5m, 1h, 1d）';
COMMENT ON COLUMN alert_rule.eval_every IS '评估频率，规则执行间隔（如：30s, 1m, 5m）';
COMMENT ON COLUMN alert_rule.severity IS '严重级别：info(信息)、warn(警告)、critical(严重)';
COMMENT ON COLUMN alert_rule.selector IS '标签选择器，用于过滤监控目标（JSON格式）';
COMMENT ON COLUMN alert_rule.for_times IS '连续匹配次数，防止告警抖动';
COMMENT ON COLUMN alert_rule.enabled IS '是否启用该规则';
COMMENT ON COLUMN alert_rule.created_at IS '规则创建时间';
COMMENT ON COLUMN alert_rule.updated_at IS '规则最后更新时间';

-- 事件表注释
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

-- ========================================
-- 6. 示例数据
-- ========================================

-- 创建一些示例规则
INSERT INTO alert_rule (id, name, expression, time_window, eval_every, severity, selector, for_times) VALUES
    ('cpu_high', 'CPU使用率过高', 'cpu.usage_percent.avg > 80', '5m', '1m', 'warn', '{"host_type": "server"}', 2),
    ('memory_high', '内存使用率过高', 'memory.usage_percent.avg > 90', '5m', '1m', 'critical', '{"host_type": "server"}', 1),
    ('disk_high', '磁盘使用率过高', 'disk.usage_percent.avg > 85', '10m', '2m', 'warn', '{"host_type": "server"}', 3)
ON CONFLICT (id) DO NOTHING;

-- ========================================
-- 7. 视图
-- ========================================

-- 规则统计视图
CREATE OR REPLACE VIEW rule_statistics AS
SELECT 
    severity,
    enabled,
    COUNT(*) as rule_count,
    MIN(created_at) as oldest_rule,
    MAX(updated_at) as latest_update
FROM alert_rule
GROUP BY severity, enabled
ORDER BY severity, enabled;

-- 活跃规则视图
CREATE OR REPLACE VIEW active_rules AS
SELECT 
    id,
    name,
    expression,
    time_window,
    eval_every,
    severity,
    selector,
    for_times,
    created_at,
    updated_at
FROM alert_rule
WHERE enabled = true
ORDER BY severity DESC, created_at DESC;

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
