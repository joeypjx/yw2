-- 告警规则表设置
-- 用于存储告警系统的规则配置

-- 创建告警规则表
CREATE TABLE IF NOT EXISTS alert_rule (
    id          TEXT PRIMARY KEY,           -- 规则ID（唯一标识）
    name        TEXT NOT NULL,              -- 规则名称
    description TEXT NOT NULL DEFAULT '',   -- 规则描述
    expression  TEXT NOT NULL,              -- 告警表达式（DSL格式）
    time_window TEXT NOT NULL,              -- 时间窗口（如：5m, 1h）
    eval_every  TEXT NOT NULL,              -- 评估频率（如：30s, 1m）
    severity    TEXT NOT NULL,              -- 严重级别（info, warn, critical）
    tag         TEXT DEFAULT '',            -- 规则标签（用于分类）
    selector    JSONB,                      -- 标签选择器（用于过滤目标）
    for_times   INTEGER NOT NULL DEFAULT 1, -- 连续匹配次数（防抖动）
    enabled     BOOLEAN NOT NULL DEFAULT true, -- 是否启用
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(), -- 创建时间
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()  -- 更新时间
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled 
ON alert_rule (enabled, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_rule_severity 
ON alert_rule (severity, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_alert_rule_name 
ON alert_rule (name);

CREATE INDEX IF NOT EXISTS idx_alert_rule_tag 
ON alert_rule (tag, created_at DESC);

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
COMMENT ON TABLE alert_rule IS '告警规则表，存储告警系统的规则配置';
COMMENT ON COLUMN alert_rule.id IS '规则唯一标识符';
COMMENT ON COLUMN alert_rule.name IS '规则名称，用于描述告警内容';
COMMENT ON COLUMN alert_rule.description IS '规则详细描述，用于说明告警的具体含义和处理建议';
COMMENT ON COLUMN alert_rule.expression IS '告警表达式，使用DSL格式（如：cpu.usage_percent.avg > 80）';
COMMENT ON COLUMN alert_rule.time_window IS '时间窗口，用于聚合计算（如：5m, 1h, 1d）';
COMMENT ON COLUMN alert_rule.eval_every IS '评估频率，规则执行间隔（如：30s, 1m, 5m）';
COMMENT ON COLUMN alert_rule.severity IS '严重级别：info(信息)、warn(警告)、critical(严重)';
COMMENT ON COLUMN alert_rule.tag IS '规则标签，用于规则分类和管理';
COMMENT ON COLUMN alert_rule.selector IS '标签选择器，用于过滤监控目标（JSON格式）';
COMMENT ON COLUMN alert_rule.for_times IS '连续匹配次数，防止告警抖动';
COMMENT ON COLUMN alert_rule.enabled IS '是否启用该规则';
COMMENT ON COLUMN alert_rule.created_at IS '规则创建时间';
COMMENT ON COLUMN alert_rule.updated_at IS '规则最后更新时间';

-- 创建一些示例规则（可选）
INSERT INTO alert_rule (id, name, description, expression, time_window, eval_every, severity, tag, selector, for_times) VALUES
    ('cpu_high', 'CPU使用率过高', '当CPU使用率持续超过80%时触发告警，可能表示系统负载过高或存在性能问题', 'cpu.usage_percent.avg > 80', '5m', '1m', '一般', 'resource', '{"host_type": "server"}', 2),
    ('memory_high', '内存使用率过高', '当内存使用率超过90%时触发严重告警，可能导致系统不稳定或服务中断', 'memory.usage_percent.avg > 90', '5m', '1m', '严重', 'resource', '{"host_type": "server"}', 1),
    ('disk_high', '磁盘使用率过高', '当磁盘使用率超过85%时触发告警，建议及时清理或扩容存储空间', 'disk.usage_percent.avg > 85', '10m', '2m', '一般', 'storage', '{"host_type": "server"}', 3),
    ('node_offline', '节点离线', '节点 {{host_ip}} 心跳在5秒窗口内未出现，判定离线', 'alive.alive.max == 0', '5s', '10s', '严重', 'availability', NULL, 1)
ON CONFLICT (id) DO NOTHING;

-- 创建规则统计视图
CREATE OR REPLACE VIEW rule_statistics AS
SELECT 
    severity,
    tag,
    enabled,
    COUNT(*) as rule_count,
    MIN(created_at) as oldest_rule,
    MAX(updated_at) as latest_update
FROM alert_rule
GROUP BY severity, tag, enabled
ORDER BY severity, tag, enabled;

-- 创建活跃规则视图
CREATE OR REPLACE VIEW active_rules AS
SELECT 
    id,
    name,
    expression,
    time_window,
    eval_every,
    severity,
    tag,
    selector,
    for_times,
    created_at,
    updated_at
FROM alert_rule
WHERE enabled = true
ORDER BY severity DESC, tag, created_at DESC;
