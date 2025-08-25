-- 告警系统测试设置
-- 只包含基本的表结构，用于测试

-- 1. 告警规则表
CREATE TABLE IF NOT EXISTS alert_rule (
    id          TEXT PRIMARY KEY,
    name        TEXT NOT NULL,
    expression  TEXT NOT NULL,
    time_window TEXT NOT NULL,
    eval_every  TEXT NOT NULL,
    severity    TEXT NOT NULL,
    selector    JSONB,
    for_times   INTEGER NOT NULL DEFAULT 1,
    enabled     BOOLEAN NOT NULL DEFAULT true,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- 2. 告警事件表
CREATE TABLE IF NOT EXISTS alert_event (
    time        TIMESTAMPTZ NOT NULL,
    fingerprint TEXT NOT NULL,
    rule_id     TEXT NOT NULL,
    action      TEXT NOT NULL,
    status      TEXT NOT NULL,
    severity    TEXT NOT NULL,
    labels      JSONB,
    title       TEXT,
    description TEXT,
    value       DOUBLE PRECISION,
    unit        TEXT,
    context     JSONB
);

-- 3. 基本索引
CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled ON alert_rule (enabled, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_event_time ON alert_event (time DESC);
CREATE INDEX IF NOT EXISTS idx_alert_event_fingerprint ON alert_event (fingerprint, time DESC);

-- 4. 示例规则
INSERT INTO alert_rule (id, name, expression, time_window, eval_every, severity, selector, for_times) VALUES
    ('cpu_high', 'CPU使用率过高', 'cpu.usage_percent.avg > 80', '5m', '1m', 'warn', '{"host_type": "server"}', 2)
ON CONFLICT (id) DO NOTHING;

-- 5. 测试查询
SELECT 'Tables created successfully' as status;
SELECT COUNT(*) as rule_count FROM alert_rule;
SELECT COUNT(*) as event_count FROM alert_event;
