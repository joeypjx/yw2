-- AlertV2 告警规则表创建脚本（修复版本）
-- 用于在PostgreSQL数据库中创建告警规则存储表
-- 使用方法：psql "postgres://user:password@host:5432/dbname" -f alert_rule_setup_fixed.sql

-- 创建告警规则表
CREATE TABLE IF NOT EXISTS alert_rule (
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
    created_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW(),         -- 创建时间，系统自动生成
    updated_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW()          -- 更新时间，系统自动生成
);

-- 创建基础索引
CREATE INDEX IF NOT EXISTS idx_alert_rule_alert_name ON alert_rule(alert_name);
CREATE INDEX IF NOT EXISTS idx_alert_rule_alert_type ON alert_rule(alert_type);
CREATE INDEX IF NOT EXISTS idx_alert_rule_severity ON alert_rule(severity);
CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled ON alert_rule(enabled);
CREATE INDEX IF NOT EXISTS idx_alert_rule_created_at ON alert_rule(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rule_updated_at ON alert_rule(updated_at DESC);

-- 创建JSONB字段的索引（使用B-tree索引）
CREATE INDEX IF NOT EXISTS idx_alert_rule_expression_stable ON alert_rule ((expression->>'stable'));
CREATE INDEX IF NOT EXISTS idx_alert_rule_expression_metric ON alert_rule ((expression->>'metric'));

-- 创建复合索引
CREATE INDEX IF NOT EXISTS idx_alert_rule_type_severity ON alert_rule(alert_type, severity);
CREATE INDEX IF NOT EXISTS idx_alert_rule_type_created ON alert_rule(alert_type, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled_type ON alert_rule(enabled, alert_type);

-- 创建触发器函数，自动更新updated_at字段
CREATE OR REPLACE FUNCTION update_alert_rule_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- 创建触发器
CREATE TRIGGER trigger_update_alert_rule_updated_at
    BEFORE UPDATE ON alert_rule
    FOR EACH ROW
    EXECUTE FUNCTION update_alert_rule_updated_at();

-- 插入示例数据
INSERT INTO alert_rule (
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

INSERT INTO alert_rule (
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

INSERT INTO alert_rule (
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
    '10s',
    '严重',
    '节点离线',
    '节点{{host_ip}}心跳超时',
    '可用性',
    TRUE,
    NOW(),
    NOW()
) ON CONFLICT (id) DO NOTHING;
