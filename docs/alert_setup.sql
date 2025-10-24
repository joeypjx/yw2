-- AlertV2 告警存储表创建脚本
-- 用于在PostgreSQL数据库中创建告警存储表
-- 使用方法：psql "postgres://user:password@host:5432/dbname" -f alert_setup.sql

-- 创建告警表
CREATE TABLE IF NOT EXISTS alert (
    -- 主键和标识
    id                  VARCHAR(200)    PRIMARY KEY,                    -- 告警ID，系统生成的唯一标识符
    fingerprint         VARCHAR(500)    NOT NULL,                       -- 告警指纹，用于去重和识别
    
    -- 告警内容
    labels              JSONB           NOT NULL,                       -- 告警标签，JSON格式
    annotations         JSONB           NOT NULL,                       -- 告警注释，JSON格式
    
    -- 时间戳
    created_at          TIMESTAMP       NOT NULL DEFAULT NOW(),         -- 第一次匹配时间
    starts_at           TIMESTAMP,                                      -- 第一次正式触发告警时间
    updated_at          TIMESTAMP       NOT NULL DEFAULT NOW(),         -- 触发中持续匹配的更新时间
    ends_at             TIMESTAMP,                                      -- 告警已解决的时间
    
    -- 告警状态
    status              VARCHAR(20)     NOT NULL DEFAULT 'pending',     -- 告警状态：pending/firing/resolved
    
    -- 关联信息
    alert_rule_id       VARCHAR(100),                                  -- 关联的告警规则ID
    host_ip             VARCHAR(50),                                   -- 节点IP，用于快速查询
    
    -- 约束
    CONSTRAINT chk_status CHECK (status IN ('pending', 'firing', 'resolved'))
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_alert_fingerprint ON alert(fingerprint);
CREATE INDEX IF NOT EXISTS idx_alert_status ON alert(status);
CREATE INDEX IF NOT EXISTS idx_alert_host_ip ON alert(host_ip);
CREATE INDEX IF NOT EXISTS idx_alert_created_at ON alert(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_starts_at ON alert(starts_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_updated_at ON alert(updated_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_ends_at ON alert(ends_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rule_id ON alert(alert_rule_id);

-- 创建复合索引
CREATE INDEX IF NOT EXISTS idx_alert_status_host ON alert(status, host_ip);
CREATE INDEX IF NOT EXISTS idx_alert_status_created ON alert(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alert_rule_status ON alert(alert_rule_id, status);

-- 创建JSONB字段的索引
CREATE INDEX IF NOT EXISTS idx_alert_labels_alert_name ON alert ((labels->>'alert_name'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_alert_type ON alert ((labels->>'alert_type'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_severity ON alert ((labels->>'severity'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_metric ON alert ((labels->>'metric'));
CREATE INDEX IF NOT EXISTS idx_alert_labels_stable ON alert ((labels->>'stable'));

-- 创建触发器函数，自动更新updated_at字段
CREATE OR REPLACE FUNCTION update_alert_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- 创建触发器
CREATE TRIGGER trigger_update_alert_updated_at
    BEFORE UPDATE ON alert
    FOR EACH ROW
    EXECUTE FUNCTION update_alert_updated_at();

-- 添加表注释
COMMENT ON TABLE alert IS '告警表，存储系统中产生的所有告警';
COMMENT ON COLUMN alert.id IS '告警ID，系统生成的唯一标识符';
COMMENT ON COLUMN alert.fingerprint IS '告警指纹，用于去重和识别，格式为alert_name|key1=value1|key2=value2';
COMMENT ON COLUMN alert.labels IS '告警标签，JSON格式，包含节点IP、指标值、告警规则信息等';
COMMENT ON COLUMN alert.annotations IS '告警注释，JSON格式，包含summary和description';
COMMENT ON COLUMN alert.created_at IS '第一次匹配时间，可能还未满足持续时间条件';
COMMENT ON COLUMN alert.starts_at IS '第一次正式触发告警时间，满足持续时间条件';
COMMENT ON COLUMN alert.updated_at IS '触发中持续匹配的更新时间，每次更新';
COMMENT ON COLUMN alert.ends_at IS '告警已解决的时间';
COMMENT ON COLUMN alert.status IS '告警状态：pending(匹配但未满足持续时间)、firing(触发告警)、resolved(已解决)';
COMMENT ON COLUMN alert.alert_rule_id IS '关联的告警规则ID';
COMMENT ON COLUMN alert.host_ip IS '节点IP，用于快速查询';

-- 插入示例数据（可选）
INSERT INTO alert (
    id, fingerprint, labels, annotations, created_at, starts_at, updated_at, 
    status, alert_rule_id, host_ip
) VALUES (
    'alert_20251023_150000_001_1234',
    'CPU使用率告警|host_ip=192.168.1.100',
    '{
        "alert_name": "CPU使用率告警",
        "alert_type": "硬件资源",
        "host_ip": "192.168.1.100",
        "metric": "usage_percent",
        "severity": "严重",
        "stable": "cpu",
        "value": "85.500000"
    }'::jsonb,
    '{
        "summary": "CPU使用率过高",
        "description": "CPU使用率超过80%"
    }'::jsonb,
    NOW(),
    NOW(),
    NOW(),
    'firing',
    'rule_20250101_120000_123_5678',
    '192.168.1.100'
) ON CONFLICT (id) DO NOTHING;

INSERT INTO alert (
    id, fingerprint, labels, annotations, created_at, starts_at, updated_at, 
    status, alert_rule_id, host_ip
) VALUES (
    'alert_20251023_150001_002_5678',
    '磁盘使用率告警|host_ip=192.168.1.101|mount_point=/data',
    '{
        "alert_name": "磁盘使用率告警",
        "alert_type": "硬件资源",
        "host_ip": "192.168.1.101",
        "metric": "usage_percent",
        "severity": "严重",
        "stable": "disk",
        "mount_point": "/data",
        "value": "92.300000"
    }'::jsonb,
    '{
        "summary": "磁盘使用率过高",
        "description": "磁盘/data使用率超过85%"
    }'::jsonb,
    NOW(),
    NOW(),
    NOW(),
    'firing',
    'rule_20250101_120000_123_5678',
    '192.168.1.101'
) ON CONFLICT (id) DO NOTHING;

INSERT INTO alert (
    id, fingerprint, labels, annotations, created_at, starts_at, updated_at, 
    status, alert_rule_id, host_ip
) VALUES (
    'alert_20251023_150002_003_9012',
    '节点离线告警|host_ip=192.168.1.102',
    '{
        "alert_name": "节点离线告警",
        "alert_type": "可用性",
        "host_ip": "192.168.1.102",
        "metric": "alive",
        "severity": "严重",
        "stable": "alive",
        "value": "0.000000"
    }'::jsonb,
    '{
        "summary": "节点离线",
        "description": "节点192.168.1.102心跳超时"
    }'::jsonb,
    NOW(),
    NOW(),
    NOW(),
    'resolved',
    'rule_20250101_120002_125_5680',
    '192.168.1.102'
) ON CONFLICT (id) DO NOTHING;
