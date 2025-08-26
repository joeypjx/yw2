1 把postgre头文件放入build，添加进头文件路径
2 功能模块都编译成静态库
3 通过功能模块-单元图，生成代码结构

4 分页

bugs
1 /node/metrics 时间矫正

/node
/node?box_id
/node?host_ip
/node/metrics
/node/historical-metrics?host_ip=192.168.66.5&time_range=2m&metrics=cpu,memory,gpu,sensor

  {
    "alert_name": "¸æ¾¯¹æÔòÃû³Æ",  -- id
    "expression": {                  -- expression: 
      "stable": "³¬¼¶±íÃû³Æ",
      "metric": "Ö¸±êÃû³Æ",
      "conditions": [
        {
          "operator": "²Ù×÷·û",
          "threshold": ãÐÖµ
        }
      ],
      "tags": [
        {
          "±êÇ©Ãû": "±êÇ©Öµ"
        }
      ]
    },
    "for": "³ÖÐøÊ±¼ä", -- eval_every * for_times
    "severity": "ÑÏÖØµÈ¼¶", -- severity
    "summary": "¸æ¾¯ÕªÒª", -- name
    "description": "¸æ¾¯ÏêÏ¸ÃèÊö",  -- description
    "alert_type": "¸æ¾¯ÀàÐÍ" --
  }
