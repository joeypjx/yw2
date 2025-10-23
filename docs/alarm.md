创建告警规则
http://192.168.10.58:18888/alarm/rules
{"alert_name":"规则标识","expression":{"stable":"disk","metric":"total","conditions":[{"operator":">","threshold":85}],"tags":[{"mount_point":"/data"}]},"for":"5s","severity":"严重","summary":"告警摘要","description":"","alert_type":"硬件资源"}
{
  "api_version": 1,
  "data": {
    "id": "规则标识",
    "message": "Rule created/updated successfully"
  },
  "status": "success"
}

获取告警规则
get　http://192.168.10.58:18888/alarm/rules
{
  "api_version": 1,
  "data": [
    {
      "alert_name": "cpu_high",
      "alert_type": "resource",
      "created_at": "2025-10-23 07:09:13.340371+00",
      "description": "当CPU使用率持续超过80%时触发告警，可能表示系统负载过高或存在性能问题",
      "enabled": true,
      "expression": {
        "conditions": [
          {
            "operator": ">",
            "threshold": 80.0
          }
        ],
        "metric": "usage_percent",
        "stable": "cpu",
        "tags": [
          {
            "host_type": "server"
          }
        ]
      },
      "for": "2m",
      "id": "cpu_high",
      "severity": "一般",
      "summary": "CPU使用率过高",
      "updated_at": "2025-10-23 07:09:13.340371+00"
    },
    {
      "alert_name": "memory_high",
      "alert_type": "resource",
      "created_at": "2025-10-23 07:09:13.340371+00",
      "description": "当内存使用率超过90%时触发严重告警，可能导致系统不稳定或服务中断",
      "enabled": true,
      "expression": {
        "conditions": [
          {
            "operator": ">",
            "threshold": 90.0
          }
        ],
        "metric": "usage_percent",
        "stable": "memory",
        "tags": [
          {
            "host_type": "server"
          }
        ]
      },
      "for": "1m",
      "id": "memory_high",
      "severity": "严重",
      "summary": "内存使用率过高",
      "updated_at": "2025-10-23 07:09:13.340371+00"
    },
    {
      "alert_name": "disk_high",
      "alert_type": "storage",
      "created_at": "2025-10-23 07:09:13.340371+00",
      "description": "当磁盘使用率超过85%时触发告警，建议及时清理或扩容存储空间",
      "enabled": true,
      "expression": {
        "conditions": [
          {
            "operator": ">",
            "threshold": 85.0
          }
        ],
        "metric": "usage_percent",
        "stable": "disk",
        "tags": [
          {
            "host_type": "server"
          }
        ]
      },
      "for": "6m",
      "id": "disk_high",
      "severity": "一般",
      "summary": "磁盘使用率过高",
      "updated_at": "2025-10-23 07:09:13.340371+00"
    },
    {
      "alert_name": "node_offline",
      "alert_type": "availability",
      "created_at": "2025-10-23 07:09:13.340371+00",
      "description": "节点 {{host_ip}} 心跳在5秒窗口内未出现，判定离线",
      "enabled": true,
      "expression": {
        "conditions": [
          {
            "operator": "==",
            "threshold": 0.0
          }
        ],
        "metric": "alive",
        "stable": "alive",
        "tags": []
      },
      "for": "10s",
      "id": "node_offline",
      "severity": "严重",
      "summary": "节点离线",
      "updated_at": "2025-10-23 07:09:13.340371+00"
    }
  ],
  "status": "success"
}

post http://192.168.10.58:18888/alarm/rules/memory_high/update
{"alert_name":"memory_high","expression":{"stable":"memory","metric":"usage_percent","conditions":[{"operator":">","threshold":90}]},"for":"1m","severity":"严重","summary":"内存使用率过高","description":"当内存使用率超过90%时触发严重告警，可能导致系统不稳定或服务中断","alert_type":"resource","enabled":true}
{
  "api_version": 1,
  "data": {
    "id": "memory_high",
    "message": "Rule updated successfully"
  },
  "status": "success"
}

get http://192.168.10.58:18888/alarm/rules/cpu_high
{
  "api_version": 1,
  "data": {
    "alert_name": "cpu_high",
    "alert_type": "resource",
    "created_at": "2025-10-23 07:09:13.340371+00",
    "description": "当CPU使用率持续超过80%时触发告警，可能表示系统负载过高或存在性能问题",
    "enabled": true,
    "expression": {
      "conditions": [
        {
          "operator": ">",
          "threshold": 80.0
        }
      ],
      "metric": "usage_percent",
      "stable": "cpu",
      "tags": [
        {
          "host_type": "server"
        }
      ]
    },
    "for": "2m",
    "id": "cpu_high",
    "severity": "一般",
    "summary": "CPU使用率过高",
    "updated_at": "2025-10-23 07:09:13.340371+00"
  },
  "status": "success"
}

http://192.168.10.58:18888/alarm/rules/cpu_high/delete
{
  "api_version": 1,
  "data": {
    "id": "cpu_high"
  },
  "status": "success"
}

get http://192.168.10.58:18888/alarm/events?limit=20
{
  "api_version": 1,
  "data": [
    {
      "annotations": {
        "description": "",
        "summary": "告警摘要"
      },
      "created_at": "2025-10-23 15:59:01",
      "ends_at": "",
      "fingerprint": "规则标识|device=/dev/sda5|mount_point=/data|host_ip=192.168.10.58/32",
      "id": "规则标识|device=/dev/sda5|mount_point=/data|host_ip=192.168.10.58/32",
      "labels": {
        "alert_type": "硬件资源",
        "alertname": "规则标识",
        "host_ip": "192.168.10.58",
        "metrics": "total",
        "severity": "严重",
        "value": "426474012672.000000"
      },
      "starts_at": "2025-10-23 15:59:01",
      "status": "firing",
      "updated_at": "2025-10-23 15:59:01"
    },
    {
      "annotations": {
        "description": "节点 192.168.2.5 心跳在5秒窗口内未出现，判定离线",
        "summary": "节点离线"
      },
      "created_at": "2025-10-23 15:10:07",
      "ends_at": "",
      "fingerprint": "node_offline|host_ip=192.168.2.5",
      "id": "node_offline|host_ip=192.168.2.5",
      "labels": {
        "alert_type": "availability",
        "alertname": "node_offline",
        "host_ip": "192.168.2.5",
        "metrics": "alive",
        "severity": "严重",
        "value": "0.000000"
      },
      "starts_at": "2025-10-23 15:10:07",
      "status": "firing",
      "updated_at": "2025-10-23 15:10:07"
    },
    {
      "annotations": {
        "description": "节点 192.168.127.133 心跳在5秒窗口内未出现，判定离线",
        "summary": "节点离线"
      },
      "created_at": "2025-10-23 15:10:07",
      "ends_at": "",
      "fingerprint": "node_offline|host_ip=192.168.127.133",
      "id": "node_offline|host_ip=192.168.127.133",
      "labels": {
        "alert_type": "availability",
        "alertname": "node_offline",
        "host_ip": "192.168.127.133",
        "metrics": "alive",
        "severity": "严重",
        "value": "0.000000"
      },
      "starts_at": "2025-10-23 15:10:07",
      "status": "firing",
      "updated_at": "2025-10-23 15:10:07"
    },
    {
      "annotations": {
        "description": "节点 192.168.66.37 心跳在5秒窗口内未出现，判定离线",
        "summary": "节点离线"
      },
      "created_at": "2025-10-23 15:10:07",
      "ends_at": "",
      "fingerprint": "node_offline|host_ip=192.168.66.37",
      "id": "node_offline|host_ip=192.168.66.37",
      "labels": {
        "alert_type": "availability",
        "alertname": "node_offline",
        "host_ip": "192.168.66.37",
        "metrics": "alive",
        "severity": "严重",
        "value": "0.000000"
      },
      "starts_at": "2025-10-23 15:10:07",
      "status": "firing",
      "updated_at": "2025-10-23 15:10:07"
    }
  ],
  "status": "success"
}

http://192.168.10.58:18888/alarm/events/count?status=firing



