# 文档整理说明

## 🗑️ **已删除的过时文档**

### **调试和临时文件**
- `error` - 编译错误信息
- `todo` - 编译错误日志
- `ipmitoollog.txt` - IPMI 工具调试日志
- `ipmi-lan-session-common.c` - IPMI 调试代码文件

### **过时的重构计划文档**
- `alert_refactor_plan.md` - 告警重构计划（已完成）
- `alert_state_simplification.md` - AlertState 简化说明（已完成）
- `alert_event_simplified.md` - AlertEvent 简化说明（已完成）
- `alert_rule_simplified.md` - AlertRule 简化说明（已完成）
- `single_event_state_update.md` - 单事件状态更新说明（已完成）
- `memory_event_repository_usage.md` - MemoryEventRepository 使用说明（已完成）

### **过时的数据库脚本**
- `alert_event_setup_basic.sql` - 旧版告警事件表（已被 v2 版本替代）
- `alert_rule_setup.sql` - 旧版告警规则表（已被 v2 版本替代）

### **过时的待办事项**
- `todo.md` - 旧的待办事项列表

## 📁 **保留的核心文档**

### **告警系统文档**
- `alert_event_retrigger_fix.md` - 告警事件重新触发修复说明
- `alert_event_setup_v2.sql` - 新版告警事件表结构
- `alert_rule_setup_v2.sql` - 新版告警规则表结构
- `alert_state_further_optimization.md` - AlertState 进一步优化说明

### **API 文档**
- `api/` - API 规范目录
  - `heartbeat.yaml` - 心跳 API 规范
  - `resource.yaml` - 资源 API 规范
- `api_data_format.md` - API 数据格式说明
- `api.md` - API 概览
- `interface_specification.md` - 接口规范详细说明

### **系统文档**
- `module.md` - 模块架构说明
- `sql.md` - SQL 相关说明
- `timescaledb_setup.sql` - TimescaleDB 设置脚本
- `bmc_timescaledb_setup.sql` - BMC TimescaleDB 设置脚本
- `xtxk.md` - 系统相关说明
- `notes.md` - 重要命令和配置记录

## 📊 **整理结果**

- **删除文件**: 12 个
- **保留文件**: 16 个
- **文档目录**: 1 个（api/）

## ✅ **整理效果**

1. **清理了调试文件**: 移除了所有编译错误、调试日志等临时文件
2. **删除了过时文档**: 移除了已完成的重构计划文档
3. **保留了核心文档**: 保留了系统架构、API 规范、数据库结构等重要文档
4. **结构更清晰**: 文档结构更加清晰，便于维护和查找

整理后的文档结构更加简洁明了，只保留了当前系统需要的核心文档。
