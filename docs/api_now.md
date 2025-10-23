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


http://192.168.10.58:18888/node
{
  "api_version": 1,
  "data": {
    "nodes": [
      {
        "board_type": "GPU",
        "box_id": 0,
        "box_type": "计算I型",
        "component": [],
        "cpu_arch": "aarch64",
        "cpu_id": 1,
        "cpu_type": " D2000 ",
        "gpu": [
          {
            "index": 0,
            "name": " Iluvatar MR-V50A"
          }
        ],
        "host_ip": "192.168.10.58",
        "hostname": "localhost.localdomain",
        "manufacturer": "715",
        "os_type": "UOS Server 20 Military",
        "production_date": "2024-05-11",
        "resource_type": "GPU I",
        "serial_number": "030MPV10E5000831",
        "slot_id": 0,
        "status": "online",
        "updated_at": 1761211760113
      }
    ]
  },
  "status": "success"
}


http://192.168.10.58:18888/node?box_id=33
{
  "api_version": 1,
  "data": {
    "nodes": []
  },
  "status": "success"
}

http://192.168.10.58:18888/node/metrics
{
  "api_version": 1,
  "data": {
    "nodes_metrics": [
      {
        "bmc_company": 0,
        "bmc_version": "",
        "board_type": "",
        "box_id": 0,
        "box_type": "",
        "component": [],
        "cpu_arch": "",
        "cpu_id": 1,
        "cpu_type": "",
        "host_ip": "192.168.10.58",
        "hostname": "",
        "id": 0,
        "ipmb_address": 0,
        "latest_container_metrics": {
          "container_count": 0,
          "paused_count": 0,
          "running_count": 0,
          "stopped_count": 0,
          "timestamp": 1761211845
        },
        "latest_cpu_metrics": {
          "core_allocated": 0,
          "core_count": 8,
          "current": 0.0,
          "load_avg_15m": 7.62,
          "load_avg_1m": 10.32,
          "load_avg_5m": 9.93,
          "power": 0.0,
          "temperature": 0.0,
          "timestamp": 1761211845,
          "usage_percent": 64.98316498316498,
          "voltage": 0.0
        },
        "latest_disk_metrics": {
          "disk_count": 2,
          "disks": [
            {
              "device": "/dev/sda4",
              "free": 46964076544,
              "mount_point": "/",
              "timestamp": 1761211845,
              "total": 75125227520,
              "usage_percent": 37.48561156570591,
              "used": 28161150976
            },
            {
              "device": "/dev/sda5",
              "free": 351919628288,
              "mount_point": "/data",
              "timestamp": 1761211845,
              "total": 426474012672,
              "usage_percent": 17.481577345567263,
              "used": 74554384384
            }
          ],
          "timestamp": 1761211845
        },
        "latest_gpu_metrics": {
          "gpu_count": 1,
          "gpus": [
            {
              "compute_usage": 0.0,
              "current": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 20.0,
              "temperature": 45.0,
              "timestamp": 1761211845,
              "voltage": 0.0
            }
          ],
          "timestamp": 1761211845
        },
        "latest_memory_metrics": {
          "free": 8114470912,
          "timestamp": 1761211845,
          "total": 15645540352,
          "usage_percent": 48.13556624164335,
          "used": 7531069440
        },
        "latest_network_metrics": {
          "network_count": 2,
          "networks": [
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188401652,
              "rx_errors": 0,
              "rx_packets": 13810021,
              "rx_rate": 16058,
              "timestamp": 1761211845,
              "tx_bytes": 1978095272,
              "tx_errors": 0,
              "tx_packets": 3260419,
              "tx_rate": 8690
            },
            {
              "interface": "docker0",
              "rx_bytes": 59223458,
              "rx_errors": 0,
              "rx_packets": 476990,
              "rx_rate": 2812,
              "timestamp": 1761211845,
              "tx_bytes": 110288783,
              "tx_errors": 0,
              "tx_packets": 579150,
              "tx_rate": 3074
            }
          ],
          "timestamp": 1761211845
        },
        "latest_sensor_metrics": {
          "sensor_count": 0,
          "sensors": [],
          "timestamp": 1761211845
        },
        "module_type": 0,
        "os_type": "",
        "resource_type": "",
        "service_port": 0,
        "slot_id": 0,
        "srio_id": 0,
        "status": "online",
        "updated_at": 1761211844161
      }
    ]
  },
  "status": "success"
}

http://192.168.10.58:18888/node/historical-metrics?host_ip=192.168.10.58&time_range=5m&metrics=cpu,memory,gpu,disk,network,sensor
{
  "api_version": 1,
  "data": {
    "historical_metrics": {
      "box_id": 0,
      "cpu_id": 1,
      "host_ip": "192.168.10.58",
      "metrics": {
        "cpu": [
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211580,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211590,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211600,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211610,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211620,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211630,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211640,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211650,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211660,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211670,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 0,
            "core_count": 0,
            "current": 0.0,
            "load_avg_15m": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211680,
            "usage_percent": 0.0,
            "voltage": 0.0
          },
          {
            "core_allocated": 2,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.046666666666668,
            "load_avg_1m": 8.952222222222224,
            "load_avg_5m": 9.33222222222222,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211690,
            "usage_percent": 79.65936284595003,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.090833333333332,
            "load_avg_1m": 9.298333333333332,
            "load_avg_5m": 9.399166666666668,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211700,
            "usage_percent": 71.32616554725463,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.066666666666666,
            "load_avg_1m": 8.645555555555555,
            "load_avg_5m": 9.255555555555553,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211710,
            "usage_percent": 72.27127786258636,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.076666666666668,
            "load_avg_1m": 8.56111111111111,
            "load_avg_5m": 9.217777777777776,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211720,
            "usage_percent": 72.81518451273217,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.223333333333333,
            "load_avg_1m": 10.33083333333333,
            "load_avg_5m": 9.579999999999998,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211730,
            "usage_percent": 73.05208560498839,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.271111111111113,
            "load_avg_1m": 10.552222222222223,
            "load_avg_5m": 9.65,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211740,
            "usage_percent": 71.86222649367448,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.347777777777777,
            "load_avg_1m": 11.256666666666666,
            "load_avg_5m": 9.826666666666668,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211750,
            "usage_percent": 74.1695600741309,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.354166666666668,
            "load_avg_1m": 10.625,
            "load_avg_5m": 9.746666666666664,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211760,
            "usage_percent": 74.129286011981,
            "voltage": 0.0
          },
          {
            "core_allocated": 3,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.359999999999999,
            "load_avg_1m": 10.158888888888889,
            "load_avg_5m": 9.673333333333334,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211770,
            "usage_percent": 71.1938693399854,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.435555555555556,
            "load_avg_1m": 10.87111111111111,
            "load_avg_5m": 9.838888888888889,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211780,
            "usage_percent": 74.73333857503225,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.508333333333332,
            "load_avg_1m": 11.46,
            "load_avg_5m": 10.0,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211790,
            "usage_percent": 69.96313003383146,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.475555555555557,
            "load_avg_1m": 10.32,
            "load_avg_5m": 9.804444444444448,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211800,
            "usage_percent": 70.3201184263944,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.5477777777777755,
            "load_avg_1m": 11.015555555555556,
            "load_avg_5m": 9.962222222222223,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211810,
            "usage_percent": 67.92584410691114,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.640833333333334,
            "load_avg_1m": 11.783333333333333,
            "load_avg_5m": 10.166666666666668,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211820,
            "usage_percent": 65.27875769413374,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.601111111111113,
            "load_avg_1m": 10.414444444444444,
            "load_avg_5m": 9.935555555555554,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211830,
            "usage_percent": 65.20620083738598,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.610000000000001,
            "load_avg_1m": 10.213333333333333,
            "load_avg_5m": 9.906666666666666,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211840,
            "usage_percent": 62.847626262381596,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.733333333333332,
            "load_avg_1m": 11.530833333333335,
            "load_avg_5m": 10.200000000000003,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211850,
            "usage_percent": 70.70283510900238,
            "voltage": 0.0
          },
          {
            "core_allocated": 4,
            "core_count": 8,
            "current": 0.0,
            "load_avg_15m": 7.806666666666666,
            "load_avg_1m": 12.019999999999998,
            "load_avg_5m": 10.353333333333333,
            "power": 0.0,
            "temperature": 0.0,
            "timestamp": 1761211860,
            "usage_percent": 82.73925205770291,
            "voltage": 0.0
          }
        ],
        "disk": {
          "/dev/sda4": [
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211580,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211590,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211600,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211610,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211620,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211630,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211640,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211650,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211660,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211670,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 0,
              "mount_point": "/",
              "timestamp": 1761211680,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda4",
              "free": 46964210347,
              "mount_point": "/",
              "timestamp": 1761211690,
              "total": 75125227520,
              "usage_percent": 37.48543345953428,
              "used": 28161017173
            },
            {
              "device": "/dev/sda4",
              "free": 46964142080,
              "mount_point": "/",
              "timestamp": 1761211700,
              "total": 75125227520,
              "usage_percent": 37.485524330029996,
              "used": 28161085440
            },
            {
              "device": "/dev/sda4",
              "free": 46964151182,
              "mount_point": "/",
              "timestamp": 1761211710,
              "total": 75125227520,
              "usage_percent": 37.4855122139639,
              "used": 28161076338
            },
            {
              "device": "/dev/sda4",
              "free": 46964198514,
              "mount_point": "/",
              "timestamp": 1761211720,
              "total": 75125227520,
              "usage_percent": 37.4854492104202,
              "used": 28161029006
            },
            {
              "device": "/dev/sda4",
              "free": 46964215808,
              "mount_point": "/",
              "timestamp": 1761211730,
              "total": 75125227520,
              "usage_percent": 37.48542618989462,
              "used": 28161011712
            },
            {
              "device": "/dev/sda4",
              "free": 46964215808,
              "mount_point": "/",
              "timestamp": 1761211740,
              "total": 75125227520,
              "usage_percent": 37.48542618989462,
              "used": 28161011712
            },
            {
              "device": "/dev/sda4",
              "free": 46964215808,
              "mount_point": "/",
              "timestamp": 1761211750,
              "total": 75125227520,
              "usage_percent": 37.48542618989462,
              "used": 28161011712
            },
            {
              "device": "/dev/sda4",
              "free": 46964206251,
              "mount_point": "/",
              "timestamp": 1761211760,
              "total": 75125227520,
              "usage_percent": 37.485438911764014,
              "used": 28161021269
            },
            {
              "device": "/dev/sda4",
              "free": 46964182585,
              "mount_point": "/",
              "timestamp": 1761211770,
              "total": 75125227520,
              "usage_percent": 37.485470413535865,
              "used": 28161044935
            },
            {
              "device": "/dev/sda4",
              "free": 46964137529,
              "mount_point": "/",
              "timestamp": 1761211780,
              "total": 75125227520,
              "usage_percent": 37.485530388063054,
              "used": 28161089991
            },
            {
              "device": "/dev/sda4",
              "free": 46964140715,
              "mount_point": "/",
              "timestamp": 1761211790,
              "total": 75125227520,
              "usage_percent": 37.48552614743991,
              "used": 28161086805
            },
            {
              "device": "/dev/sda4",
              "free": 46964097024,
              "mount_point": "/",
              "timestamp": 1761211800,
              "total": 75125227520,
              "usage_percent": 37.48558430455719,
              "used": 28161130496
            },
            {
              "device": "/dev/sda4",
              "free": 46964096569,
              "mount_point": "/",
              "timestamp": 1761211810,
              "total": 75125227520,
              "usage_percent": 37.485584910360494,
              "used": 28161130951
            },
            {
              "device": "/dev/sda4",
              "free": 46964073813,
              "mount_point": "/",
              "timestamp": 1761211820,
              "total": 75125227520,
              "usage_percent": 37.48561520052573,
              "used": 28161153707
            },
            {
              "device": "/dev/sda4",
              "free": 46964061980,
              "mount_point": "/",
              "timestamp": 1761211830,
              "total": 75125227520,
              "usage_percent": 37.48563095141166,
              "used": 28161165540
            },
            {
              "device": "/dev/sda4",
              "free": 46964076544,
              "mount_point": "/",
              "timestamp": 1761211840,
              "total": 75125227520,
              "usage_percent": 37.48561156570591,
              "used": 28161150976
            },
            {
              "device": "/dev/sda4",
              "free": 46964087467,
              "mount_point": "/",
              "timestamp": 1761211850,
              "total": 75125227520,
              "usage_percent": 37.48559702642658,
              "used": 28161140053
            },
            {
              "device": "/dev/sda4",
              "free": 46964064256,
              "mount_point": "/",
              "timestamp": 1761211860,
              "total": 75125227520,
              "usage_percent": 37.485627922395146,
              "used": 28161163264
            }
          ],
          "/dev/sda5": [
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211580,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211590,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211600,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211610,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211620,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211630,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211640,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211650,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211660,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211670,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 0,
              "mount_point": "/data",
              "timestamp": 1761211680,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            },
            {
              "device": "/dev/sda5",
              "free": 351926081308,
              "mount_point": "/data",
              "timestamp": 1761211690,
              "total": 426474012672,
              "usage_percent": 17.480064235681848,
              "used": 74547931364
            },
            {
              "device": "/dev/sda5",
              "free": 351922195797,
              "mount_point": "/data",
              "timestamp": 1761211700,
              "total": 426474012672,
              "usage_percent": 17.48097531373014,
              "used": 74551816875
            },
            {
              "device": "/dev/sda5",
              "free": 351921035036,
              "mount_point": "/data",
              "timestamp": 1761211710,
              "total": 426474012672,
              "usage_percent": 17.481247489959966,
              "used": 74552977636
            },
            {
              "device": "/dev/sda5",
              "free": 351920625436,
              "mount_point": "/data",
              "timestamp": 1761211720,
              "total": 426474012672,
              "usage_percent": 17.481343533326694,
              "used": 74553387236
            },
            {
              "device": "/dev/sda5",
              "free": 351920507904,
              "mount_point": "/data",
              "timestamp": 1761211730,
              "total": 426474012672,
              "usage_percent": 17.481371092437204,
              "used": 74553504768
            },
            {
              "device": "/dev/sda5",
              "free": 351920495730,
              "mount_point": "/data",
              "timestamp": 1761211740,
              "total": 426474012672,
              "usage_percent": 17.481373947059495,
              "used": 74553516942
            },
            {
              "device": "/dev/sda5",
              "free": 351920372850,
              "mount_point": "/data",
              "timestamp": 1761211750,
              "total": 426474012672,
              "usage_percent": 17.481402760069514,
              "used": 74553639822
            },
            {
              "device": "/dev/sda5",
              "free": 351920276821,
              "mount_point": "/data",
              "timestamp": 1761211760,
              "total": 426474012672,
              "usage_percent": 17.481425276903273,
              "used": 74553735851
            },
            {
              "device": "/dev/sda5",
              "free": 351920277732,
              "mount_point": "/data",
              "timestamp": 1761211770,
              "total": 426474012672,
              "usage_percent": 17.481425063473566,
              "used": 74553734940
            },
            {
              "device": "/dev/sda5",
              "free": 351920056092,
              "mount_point": "/data",
              "timestamp": 1761211780,
              "total": 426474012672,
              "usage_percent": 17.481477033606453,
              "used": 74553956580
            },
            {
              "device": "/dev/sda5",
              "free": 351919976789,
              "mount_point": "/data",
              "timestamp": 1761211790,
              "total": 426474012672,
              "usage_percent": 17.481495628669403,
              "used": 74554035883
            },
            {
              "device": "/dev/sda5",
              "free": 351919815339,
              "mount_point": "/data",
              "timestamp": 1761211800,
              "total": 426474012672,
              "usage_percent": 17.48153348576312,
              "used": 74554197333
            },
            {
              "device": "/dev/sda5",
              "free": 351919732508,
              "mount_point": "/data",
              "timestamp": 1761211810,
              "total": 426474012672,
              "usage_percent": 17.481552907866174,
              "used": 74554280164
            },
            {
              "device": "/dev/sda5",
              "free": 351919751168,
              "mount_point": "/data",
              "timestamp": 1761211820,
              "total": 426474012672,
              "usage_percent": 17.48154853255724,
              "used": 74554261504
            },
            {
              "device": "/dev/sda5",
              "free": 351919768917,
              "mount_point": "/data",
              "timestamp": 1761211830,
              "total": 426474012672,
              "usage_percent": 17.48154437067802,
              "used": 74554243755
            },
            {
              "device": "/dev/sda5",
              "free": 351919678805,
              "mount_point": "/data",
              "timestamp": 1761211840,
              "total": 426474012672,
              "usage_percent": 17.481565500218696,
              "used": 74554333867
            },
            {
              "device": "/dev/sda5",
              "free": 351920077824,
              "mount_point": "/data",
              "timestamp": 1761211850,
              "total": 426474012672,
              "usage_percent": 17.481471937972277,
              "used": 74553934848
            },
            {
              "device": "/dev/sda5",
              "free": 351918193323,
              "mount_point": "/data",
              "timestamp": 1761211860,
              "total": 426474012672,
              "usage_percent": 17.48191381749537,
              "used": 74555819349
            }
          ]
        },
        "gpu": {
          "gpu_0": [
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211580
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211590
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211600
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211610
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211620
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211630
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211640
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211650
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211660
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211670
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": " Iluvatar MR-V50A",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 1761211680
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.77777777777778,
              "temperature": 44.55555555555556,
              "timestamp": 1761211690
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.916666666666668,
              "temperature": 44.666666666666664,
              "timestamp": 1761211700
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.88888888888889,
              "temperature": 44.55555555555556,
              "timestamp": 1761211710
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.555555555555557,
              "temperature": 44.44444444444444,
              "timestamp": 1761211720
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.75,
              "temperature": 44.5,
              "timestamp": 1761211730
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.77777777777778,
              "temperature": 44.666666666666664,
              "timestamp": 1761211740
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.77777777777778,
              "temperature": 44.333333333333336,
              "timestamp": 1761211750
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.75,
              "temperature": 44.333333333333336,
              "timestamp": 1761211760
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.77777777777778,
              "temperature": 44.666666666666664,
              "timestamp": 1761211770
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.88888888888889,
              "temperature": 44.666666666666664,
              "timestamp": 1761211780
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.833333333333332,
              "temperature": 44.416666666666664,
              "timestamp": 1761211790
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 20.0,
              "temperature": 44.333333333333336,
              "timestamp": 1761211800
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.77777777777778,
              "temperature": 44.55555555555556,
              "timestamp": 1761211810
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.833333333333332,
              "temperature": 44.416666666666664,
              "timestamp": 1761211820
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.88888888888889,
              "temperature": 44.666666666666664,
              "timestamp": 1761211830
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.444444444444443,
              "temperature": 44.77777777777778,
              "timestamp": 1761211840
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.416666666666668,
              "temperature": 44.333333333333336,
              "timestamp": 1761211850
            },
            {
              "compute_usage": 0.0,
              "index": 0,
              "mem_total": 17179869184,
              "mem_usage": 1.0,
              "mem_used": 119537664,
              "name": " Iluvatar MR-V50A",
              "power": 19.666666666666668,
              "temperature": 44.44444444444444,
              "timestamp": 1761211860
            }
          ]
        },
        "memory": [
          {
            "free": 0,
            "timestamp": 1761211580,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211590,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211600,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211610,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211620,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211630,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211640,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211650,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211660,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211670,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 0,
            "timestamp": 1761211680,
            "total": 0,
            "usage_percent": 0.0,
            "used": 0
          },
          {
            "free": 8219445020,
            "timestamp": 1761211690,
            "total": 15645540352,
            "usage_percent": 47.46461396973268,
            "used": 7426095332
          },
          {
            "free": 8211169280,
            "timestamp": 1761211700,
            "total": 15645540352,
            "usage_percent": 47.517509173466486,
            "used": 7434371072
          },
          {
            "free": 8208689835,
            "timestamp": 1761211710,
            "total": 15645540352,
            "usage_percent": 47.53335679059922,
            "used": 7436850517
          },
          {
            "free": 8204124160,
            "timestamp": 1761211720,
            "total": 15645540352,
            "usage_percent": 47.56253874637669,
            "used": 7441416192
          },
          {
            "free": 8206663680,
            "timestamp": 1761211730,
            "total": 15645540352,
            "usage_percent": 47.546307156141616,
            "used": 7438876672
          },
          {
            "free": 8204830492,
            "timestamp": 1761211740,
            "total": 15645540352,
            "usage_percent": 47.55802415353711,
            "used": 7440709860
          },
          {
            "free": 8209403449,
            "timestamp": 1761211750,
            "total": 15645540352,
            "usage_percent": 47.52879565556542,
            "used": 7436136903
          },
          {
            "free": 8200055467,
            "timestamp": 1761211760,
            "total": 15645540352,
            "usage_percent": 47.588544197398484,
            "used": 7445484885
          },
          {
            "free": 8201604665,
            "timestamp": 1761211770,
            "total": 15645540352,
            "usage_percent": 47.578642345577656,
            "used": 7443935687
          },
          {
            "free": 8142389248,
            "timestamp": 1761211780,
            "total": 15645540352,
            "usage_percent": 47.957123468994524,
            "used": 7503151104
          },
          {
            "free": 8096612352,
            "timestamp": 1761211790,
            "total": 15645540352,
            "usage_percent": 48.24971097297388,
            "used": 7548928000
          },
          {
            "free": 8103839516,
            "timestamp": 1761211800,
            "total": 15645540352,
            "usage_percent": 48.20351784520811,
            "used": 7541700836
          },
          {
            "free": 8109060551,
            "timestamp": 1761211810,
            "total": 15645540352,
            "usage_percent": 48.170147091950625,
            "used": 7536479801
          },
          {
            "free": 8103826773,
            "timestamp": 1761211820,
            "total": 15645540352,
            "usage_percent": 48.20359929404799,
            "used": 7541713579
          },
          {
            "free": 8107334770,
            "timestamp": 1761211830,
            "total": 15645540352,
            "usage_percent": 48.18117759198133,
            "used": 7538205582
          },
          {
            "free": 8107363897,
            "timestamp": 1761211840,
            "total": 15645540352,
            "usage_percent": 48.18099142320445,
            "used": 7538176455
          },
          {
            "free": 8103504555,
            "timestamp": 1761211850,
            "total": 15645540352,
            "usage_percent": 48.205658786142344,
            "used": 7542035797
          },
          {
            "free": 8104596821,
            "timestamp": 1761211860,
            "total": 15645540352,
            "usage_percent": 48.19867745700898,
            "used": 7540943531
          }
        ],
        "network": {
          "docker0": [
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211580,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211590,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211600,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211610,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211620,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211630,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211640,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211650,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211660,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211670,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211680,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 58655610,
              "rx_errors": 0,
              "rx_packets": 472325,
              "rx_rate": 3083,
              "timestamp": 1761211690,
              "tx_bytes": 109164219,
              "tx_errors": 0,
              "tx_packets": 573636,
              "tx_rate": 5481
            },
            {
              "interface": "docker0",
              "rx_bytes": 58696762,
              "rx_errors": 0,
              "rx_packets": 472659,
              "rx_rate": 3673,
              "timestamp": 1761211700,
              "tx_bytes": 109243579,
              "tx_errors": 0,
              "tx_packets": 574028,
              "tx_rate": 6847
            },
            {
              "interface": "docker0",
              "rx_bytes": 58734339,
              "rx_errors": 0,
              "rx_packets": 472970,
              "rx_rate": 4085,
              "timestamp": 1761211710,
              "tx_bytes": 109318975,
              "tx_errors": 0,
              "tx_packets": 574396,
              "tx_rate": 8357
            },
            {
              "interface": "docker0",
              "rx_bytes": 58769108,
              "rx_errors": 0,
              "rx_packets": 473259,
              "rx_rate": 3531,
              "timestamp": 1761211720,
              "tx_bytes": 109389111,
              "tx_errors": 0,
              "tx_packets": 574737,
              "tx_rate": 6862
            },
            {
              "interface": "docker0",
              "rx_bytes": 58808493,
              "rx_errors": 0,
              "rx_packets": 473582,
              "rx_rate": 3393,
              "timestamp": 1761211730,
              "tx_bytes": 109466430,
              "tx_errors": 0,
              "tx_packets": 575120,
              "tx_rate": 6140
            },
            {
              "interface": "docker0",
              "rx_bytes": 58846757,
              "rx_errors": 0,
              "rx_packets": 473895,
              "rx_rate": 3321,
              "timestamp": 1761211740,
              "tx_bytes": 109541099,
              "tx_errors": 0,
              "tx_packets": 575492,
              "tx_rate": 5976
            },
            {
              "interface": "docker0",
              "rx_bytes": 58880510,
              "rx_errors": 0,
              "rx_packets": 474174,
              "rx_rate": 3244,
              "timestamp": 1761211750,
              "tx_bytes": 109608148,
              "tx_errors": 0,
              "tx_packets": 575822,
              "tx_rate": 5966
            },
            {
              "interface": "docker0",
              "rx_bytes": 58919125,
              "rx_errors": 0,
              "rx_packets": 474491,
              "rx_rate": 3062,
              "timestamp": 1761211760,
              "tx_bytes": 109683990,
              "tx_errors": 0,
              "tx_packets": 576198,
              "tx_rate": 5547
            },
            {
              "interface": "docker0",
              "rx_bytes": 58959778,
              "rx_errors": 0,
              "rx_packets": 474821,
              "rx_rate": 4151,
              "timestamp": 1761211770,
              "tx_bytes": 109762677,
              "tx_errors": 0,
              "tx_packets": 576587,
              "tx_rate": 7063
            },
            {
              "interface": "docker0",
              "rx_bytes": 58993819,
              "rx_errors": 0,
              "rx_packets": 475095,
              "rx_rate": 3013,
              "timestamp": 1761211780,
              "tx_bytes": 109827277,
              "tx_errors": 0,
              "tx_packets": 576911,
              "tx_rate": 5302
            },
            {
              "interface": "docker0",
              "rx_bytes": 59030476,
              "rx_errors": 0,
              "rx_packets": 475398,
              "rx_rate": 3350,
              "timestamp": 1761211790,
              "tx_bytes": 109901041,
              "tx_errors": 0,
              "tx_packets": 577269,
              "tx_rate": 6403
            },
            {
              "interface": "docker0",
              "rx_bytes": 59072756,
              "rx_errors": 0,
              "rx_packets": 475750,
              "rx_rate": 3232,
              "timestamp": 1761211800,
              "tx_bytes": 109987280,
              "tx_errors": 0,
              "tx_packets": 577684,
              "tx_rate": 6090
            },
            {
              "interface": "docker0",
              "rx_bytes": 59105837,
              "rx_errors": 0,
              "rx_packets": 476022,
              "rx_rate": 3138,
              "timestamp": 1761211810,
              "tx_bytes": 110053431,
              "tx_errors": 0,
              "tx_packets": 578006,
              "tx_rate": 5938
            },
            {
              "interface": "docker0",
              "rx_bytes": 59143886,
              "rx_errors": 0,
              "rx_packets": 476336,
              "rx_rate": 3573,
              "timestamp": 1761211820,
              "tx_bytes": 110130154,
              "tx_errors": 0,
              "tx_packets": 578378,
              "tx_rate": 6777
            },
            {
              "interface": "docker0",
              "rx_bytes": 59186173,
              "rx_errors": 0,
              "rx_packets": 476682,
              "rx_rate": 3768,
              "timestamp": 1761211830,
              "tx_bytes": 110213941,
              "tx_errors": 0,
              "tx_packets": 578788,
              "tx_rate": 7557
            },
            {
              "interface": "docker0",
              "rx_bytes": 59220429,
              "rx_errors": 0,
              "rx_packets": 476965,
              "rx_rate": 3119,
              "timestamp": 1761211840,
              "tx_bytes": 110282905,
              "tx_errors": 0,
              "tx_packets": 579122,
              "tx_rate": 5668
            },
            {
              "interface": "docker0",
              "rx_bytes": 59257695,
              "rx_errors": 0,
              "rx_packets": 477274,
              "rx_rate": 3537,
              "timestamp": 1761211850,
              "tx_bytes": 110358290,
              "tx_errors": 0,
              "tx_packets": 579487,
              "tx_rate": 6967
            },
            {
              "interface": "docker0",
              "rx_bytes": 59295557,
              "rx_errors": 0,
              "rx_packets": 477583,
              "rx_rate": 3237,
              "timestamp": 1761211860,
              "tx_bytes": 110432808,
              "tx_errors": 0,
              "tx_packets": 579853,
              "tx_rate": 6057
            }
          ],
          "enaphyt4i0": [
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211580,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211590,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211600,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211610,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211620,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211630,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211640,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211650,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211660,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211670,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 1761211680,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9186164636,
              "rx_errors": 0,
              "rx_packets": 13777094,
              "rx_rate": 16807,
              "timestamp": 1761211690,
              "tx_bytes": 1976766915,
              "tx_errors": 0,
              "tx_packets": 3253110,
              "tx_rate": 10569
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9186311012,
              "rx_errors": 0,
              "rx_packets": 13779260,
              "rx_rate": 17545,
              "timestamp": 1761211700,
              "tx_bytes": 1976872798,
              "tx_errors": 0,
              "tx_packets": 3253581,
              "tx_rate": 9241
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9186457984,
              "rx_errors": 0,
              "rx_packets": 13781471,
              "rx_rate": 17335,
              "timestamp": 1761211710,
              "tx_bytes": 1976940625,
              "tx_errors": 0,
              "tx_packets": 3254001,
              "tx_rate": 8600
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9186576107,
              "rx_errors": 0,
              "rx_packets": 13783247,
              "rx_rate": 15687,
              "timestamp": 1761211720,
              "tx_bytes": 1976998348,
              "tx_errors": 0,
              "tx_packets": 3254367,
              "tx_rate": 7959
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9186742562,
              "rx_errors": 0,
              "rx_packets": 13785725,
              "rx_rate": 16942,
              "timestamp": 1761211730,
              "tx_bytes": 1977157594,
              "tx_errors": 0,
              "tx_packets": 3254898,
              "tx_rate": 18112
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9186895557,
              "rx_errors": 0,
              "rx_packets": 13787944,
              "rx_rate": 18649,
              "timestamp": 1761211740,
              "tx_bytes": 1977288520,
              "tx_errors": 0,
              "tx_packets": 3255420,
              "tx_rate": 11933
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187048098,
              "rx_errors": 0,
              "rx_packets": 13790126,
              "rx_rate": 17779,
              "timestamp": 1761211750,
              "tx_bytes": 1977398520,
              "tx_errors": 0,
              "tx_packets": 3255937,
              "tx_rate": 17695
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187208060,
              "rx_errors": 0,
              "rx_packets": 13792399,
              "rx_rate": 16627,
              "timestamp": 1761211760,
              "tx_bytes": 1977489869,
              "tx_errors": 0,
              "tx_packets": 3256511,
              "tx_rate": 9102
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187371669,
              "rx_errors": 0,
              "rx_packets": 13794814,
              "rx_rate": 18353,
              "timestamp": 1761211770,
              "tx_bytes": 1977562585,
              "tx_errors": 0,
              "tx_packets": 3256991,
              "tx_rate": 8114
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187510943,
              "rx_errors": 0,
              "rx_packets": 13796835,
              "rx_rate": 16718,
              "timestamp": 1761211780,
              "tx_bytes": 1977635088,
              "tx_errors": 0,
              "tx_packets": 3257487,
              "tx_rate": 9433
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187656698,
              "rx_errors": 0,
              "rx_packets": 13799039,
              "rx_rate": 16866,
              "timestamp": 1761211790,
              "tx_bytes": 1977703546,
              "tx_errors": 0,
              "tx_packets": 3257917,
              "tx_rate": 7997
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187828636,
              "rx_errors": 0,
              "rx_packets": 13801602,
              "rx_rate": 16971,
              "timestamp": 1761211800,
              "tx_bytes": 1977786390,
              "tx_errors": 0,
              "tx_packets": 3258425,
              "tx_rate": 8473
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9187973144,
              "rx_errors": 0,
              "rx_packets": 13803720,
              "rx_rate": 17146,
              "timestamp": 1761211810,
              "tx_bytes": 1977856456,
              "tx_errors": 0,
              "tx_packets": 3258898,
              "tx_rate": 8268
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188114038,
              "rx_errors": 0,
              "rx_packets": 13805790,
              "rx_rate": 14689,
              "timestamp": 1761211820,
              "tx_bytes": 1977929041,
              "tx_errors": 0,
              "tx_packets": 3259397,
              "tx_rate": 8104
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188272427,
              "rx_errors": 0,
              "rx_packets": 13808091,
              "rx_rate": 18264,
              "timestamp": 1761211830,
              "tx_bytes": 1978021076,
              "tx_errors": 0,
              "tx_packets": 3259989,
              "tx_rate": 11069
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188391927,
              "rx_errors": 0,
              "rx_packets": 13809882,
              "rx_rate": 13864,
              "timestamp": 1761211840,
              "tx_bytes": 1978090559,
              "tx_errors": 0,
              "tx_packets": 3260390,
              "tx_rate": 9414
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188509869,
              "rx_errors": 0,
              "rx_packets": 13811700,
              "rx_rate": 13861,
              "timestamp": 1761211850,
              "tx_bytes": 1978203304,
              "tx_errors": 0,
              "tx_packets": 3260846,
              "tx_rate": 13711
            },
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188650749,
              "rx_errors": 0,
              "rx_packets": 13813788,
              "rx_rate": 14413,
              "timestamp": 1761211860,
              "tx_bytes": 1978287950,
              "tx_errors": 0,
              "tx_packets": 3261362,
              "tx_rate": 9159
            }
          ]
        },
        "sensor": {}
      },
      "slot_id": 0,
      "time_range": "5m"
    }
  },
  "status": "success"
}

http://192.168.10.58:18888/box/bmc?box_id=33&time_range=2m


