-- ============================================================================
-- Alert 模块数据库表创建脚本
-- 
-- 根据 docs/alert/database_schema.md 文档生成
-- 包含表结构创建和索引配置
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/alert/create_tables.sql
-- ============================================================================

-- 1. alert_rule - 告警规则表
-- 用于存储告警规则的配置信息，定义何时触发告警以及告警的详细信息
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
    alert_type          VARCHAR(100)    NOT NULL,                       -- 告警类型，用户自定义（如 "硬件状态", "业务链路", "系统告警"）
    enabled             BOOLEAN         NOT NULL DEFAULT TRUE,          -- 是否启用，默认为true
    
    -- 时间戳
    created_at          TIMESTAMP       NOT NULL DEFAULT NOW(),         -- 创建时间，系统自动生成
    updated_at          TIMESTAMP       NOT NULL DEFAULT NOW()          -- 更新时间，系统自动生成
);

-- 2. alert_event - 告警事件表
-- 用于存储系统中实际产生的告警事件，记录告警的完整生命周期
CREATE TABLE IF NOT EXISTS alert_event (
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
    
    -- 约束
    CONSTRAINT chk_status CHECK (status IN ('pending', 'firing', 'resolved'))
);

-- ============================================================================
-- 索引创建
-- ============================================================================

-- alert_rule 表索引
-- 用于查询启用的告警规则
CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled ON alert_rule(enabled) WHERE enabled = true;

-- 用于按创建时间排序查询
CREATE INDEX IF NOT EXISTS idx_alert_rule_created_at ON alert_rule(created_at DESC);

-- alert_event 表索引
-- 用于根据指纹查询告警（最频繁的查询）
CREATE INDEX IF NOT EXISTS idx_alert_event_fingerprint ON alert_event(fingerprint);

-- 用于根据状态查询告警
CREATE INDEX IF NOT EXISTS idx_alert_event_status ON alert_event(status);

-- 用于按创建时间排序查询
CREATE INDEX IF NOT EXISTS idx_alert_event_created_at ON alert_event(created_at DESC);

-- 复合索引：用于根据指纹和状态查询（WHERE fingerprint = $1 AND status = $2）
CREATE INDEX IF NOT EXISTS idx_alert_event_fingerprint_status ON alert_event(fingerprint, status);

-- 复合索引：用于根据状态查询并按创建时间排序（WHERE status = $1 ORDER BY created_at DESC）
CREATE INDEX IF NOT EXISTS idx_alert_event_status_created_at ON alert_event(status, created_at DESC);

-- 用于根据告警规则ID查询
CREATE INDEX IF NOT EXISTS idx_alert_event_alert_rule_id ON alert_event(alert_rule_id) WHERE alert_rule_id IS NOT NULL;

-- JSONB 表达式索引：用于查询 labels 中的 host_ip（B-tree 索引用于字符串值）
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_host_ip ON alert_event((labels->>'host_ip'));

-- JSONB 表达式索引：用于查询 labels 中的 alert_type
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_alert_type ON alert_event((labels->>'alert_type'));

-- JSONB 表达式索引：用于查询 labels 中的 severity
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_severity ON alert_event((labels->>'severity'));

-- JSONB 表达式索引：用于查询 labels 中的 box_id
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_box_id ON alert_event((labels->>'box_id'));

-- JSONB 表达式索引：用于查询 labels 中的 slot_id
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_slot_id ON alert_event((labels->>'slot_id'));

-- JSONB GIN 索引：用于复杂的 JSONB 查询（如查询 labels 中的任意键值对）
CREATE INDEX IF NOT EXISTS idx_alert_event_labels_gin ON alert_event USING GIN (labels);

-- JSONB GIN 索引：用于复杂的 JSONB 查询（如模糊匹配 description）
CREATE INDEX IF NOT EXISTS idx_alert_event_annotations_gin ON alert_event USING GIN (annotations);

