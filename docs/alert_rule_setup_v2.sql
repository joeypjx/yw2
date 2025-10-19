-- 告警规则表设置 (V2 - 精简版本)
-- 支持结构化的表达式和条件配置

-- 创建告警规则表
CREATE TABLE IF NOT EXISTS alert_rule (
    id              TEXT PRIMARY KEY,           -- 规则ID（唯一标识）
    alert_name      TEXT NOT NULL,              -- 告警规则名称
    expression      JSONB NOT NULL,             -- 表达式配置（结构化JSON）
    for_duration    TEXT NOT NULL,              -- 持续时间（如：30s, 5m）
    severity        TEXT NOT NULL,              -- 严重等级（提示/一般/严重）
    summary         TEXT NOT NULL DEFAULT '',   -- 告警摘要
    description     TEXT NOT NULL DEFAULT '',   -- 告警详细描述模板
    alert_type      TEXT NOT NULL DEFAULT 'resource', -- 告警类型
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(), -- 创建时间
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now()  -- 更新时间
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_alert_rule_severity
ON alert_rule (severity, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_rule_alert_name
ON alert_rule (alert_name);

CREATE INDEX IF NOT EXISTS idx_alert_rule_alert_type
ON alert_rule (alert_type, created_at DESC);

-- 创建 GIN 索引用于 expression JSONB 查询
CREATE INDEX IF NOT EXISTS idx_alert_rule_expression_gin
ON alert_rule USING GIN (expression);

-- 创建更新时间触发器
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

-- 添加表注释
COMMENT ON TABLE alert_rule IS '告警规则表V2，支持结构化表达式配置';
COMMENT ON COLUMN alert_rule.id IS '规则唯一标识符';
COMMENT ON COLUMN alert_rule.alert_name IS '告警规则名称';
COMMENT ON COLUMN alert_rule.expression IS '表达式配置（JSON格式），包含stable、metric、conditions、tags';
COMMENT ON COLUMN alert_rule.for_duration IS '持续时间，触发告警前需满足条件的时长（如：30s, 5m）';
COMMENT ON COLUMN alert_rule.severity IS '严重等级：提示(info)、一般(warn)、严重(critical)';
COMMENT ON COLUMN alert_rule.summary IS '告警摘要，支持变量替换';
COMMENT ON COLUMN alert_rule.description IS '告警详细描述模板，支持变量替换（如{{host_ip}}、{{value}}）';
COMMENT ON COLUMN alert_rule.alert_type IS '告警类型标签，用于分类';
COMMENT ON COLUMN alert_rule.created_at IS '规则创建时间';
COMMENT ON COLUMN alert_rule.updated_at IS '规则最后更新时间';

-- 创建一些示例规则
INSERT INTO alert_rule (id, alert_name, expression, for_duration, severity, summary, description, alert_type) VALUES
    (
        'cpu_high',
        'CPU使用率过高',
        '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 80}], "tags": [{"host_type": "server"}]}'::jsonb,
        '5m',
        '一般',
        'CPU使用率超过80%',
        '主机{{host_ip}}的CPU使用率为{{value}}%，持续超过5分钟',
        'resource'
    ),
    (
        'memory_high',
        '内存使用率过高',
        '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 90}], "tags": [{"host_type": "server"}]}'::jsonb,
        '5m',
        '严重',
        '内存使用率超过90%',
        '主机{{host_ip}}的内存使用率为{{value}}%，可能导致系统不稳定',
        'resource'
    ),
    (
        'disk_high',
        '磁盘使用率过高',
        '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 85}], "tags": [{"host_type": "server"}]}'::jsonb,
        '10m',
        '一般',
        '磁盘使用率超过85%',
        '主机{{host_ip}}的磁盘{{device}}使用率为{{value}}%，建议及时清理',
        'resource'
    ),
    (
        'node_offline',
        '节点离线',
        '{"stable": "alive", "metric": "alive", "conditions": [{"operator": "==", "threshold": 0}], "tags": []}'::jsonb,
        '5s',
        '严重',
        '节点离线告警',
        '节点{{host_ip}}心跳在5秒窗口内未出现，判定为离线状态',
        'availability'
    )
ON CONFLICT (id) DO NOTHING;

-- 创建规则统计视图
CREATE OR REPLACE VIEW rule_statistics AS
SELECT
    severity,
    alert_type,
    COUNT(*) as rule_count,
    MIN(created_at) as oldest_rule,
    MAX(updated_at) as latest_update
FROM alert_rule
GROUP BY severity, alert_type
ORDER BY severity, alert_type;

-- 创建活跃规则视图
CREATE OR REPLACE VIEW active_rules AS
SELECT
    id,
    alert_name,
    expression,
    for_duration,
    severity,
    summary,
    description,
    alert_type,
    created_at,
    updated_at
FROM alert_rule
ORDER BY severity DESC, alert_type, created_at DESC;

-- 创建按资源类型统计的视图
CREATE OR REPLACE VIEW rules_by_resource AS
SELECT
    expression->>'stable' as resource_type,
    expression->>'metric' as metric_name,
    severity,
    COUNT(*) as rule_count
FROM alert_rule
WHERE expression IS NOT NULL
GROUP BY expression->>'stable', expression->>'metric', severity
ORDER BY resource_type, metric_name, severity;
