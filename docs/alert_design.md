目录 src/alert
数据库表参考文件 docs/timescaledb_setup.sql
cmake .. -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-15

（1）
已知告警规则的JSON格式如下
{
  "id": "rule_20250101_120000_123_5678",
  "alert_name": "规则标识",
  "alert_type": "硬件资源",
  "expression": {
    "stable": "disk",
    "metric": "total",
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
  },
  "for": "5s",
  "severity": "严重",
  "summary": "告警摘要",
  "description": "",
  "created_at": "2025-01-01T12:00:00.123Z",
  "updated_at": "2025-01-01T12:00:00.123Z"
}

id:系统生成的唯一标识符，格式为rule_YYYYMMDD_HHMMSS_毫秒_随机数
alert_name:告警规则标识，用户自定义
for:满足持续时间才产生告警，支持s/m/h单位
alert_type:告警类型，用户自定义
severity:告警等级，用户自定义
summary:告警摘要，一小段话，用户自定义
description:告警详情，一大段话，用户自定义，支持占位符{{}}
created_at:创建时间，系统自动生成，ISO 8601格式
updated_at:更新时间，系统自动生成，ISO 8601格式
expression:告警表达式
    stable:资源类型，包括cpu,memory,disk,network,gpu,alive，对应于时序数据库的6张表resource_xxx
    tags:标签，对应于资源表的标签列，每张表标签列不一样
    metric:指标名称，对应于资源表的指标列，每张表指标列不一样
    conditions:指标值匹配的条件，需要满足所有条件，operator支持 大于小于等于
    threshold:阈值

创建一个告警规则类

（2）当系统中创建了告警规则对象后，告警规则应该有ID、创建时间、更新时间，这些应该是系统自己生成的

（3）告警类
之后要为告警规则类添加一个告警规则评估方法，这个告警规则评估的方法会返回告警列表，表示有哪些节点匹配了告警规则的条件
告警类的JSON格式如下
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
        "alert_name": "node_offline",
        "host_ip": "192.168.127.133",
        "metric": "alive",
        "severity": "严重",
        "value": "0.000000"
      },
      "starts_at": "2025-10-23 15:10:07",
      "status": "firing",
      "updated_at": "2025-10-23 15:10:07"
    }

annotations:告警通知的内容，包括告警规则里的summary和description（占位符被填充，使用labels里的字段）
labels:告警的全量标签，包括节点IP、产生告警时的指标值、告警规则里的alert_type、severity、stable、metric、alert_name、所有tags。
created_at：第一次匹配时间（可能还未满足持续时间条件）
starts_at：第一次正式触发告警时间（满足持续时间条件）
updated_at：触发中持续匹配的更新时间（每次都更新）
ends_at：告警已解决的时间
status：告警的状态，比如 匹配但还未满足持续时间条件、触发告警、告警已解决等几种状态，告警的状态需要根据当前系统已有的告警状态来判断
fingerprint:告警指纹，包括告警规则标识、节点IP、告警规则条件标签
id:本次告警的ID,系统自动生成

（4）告警规则评估方法
告警规则评估方法需要依赖两个方法
一个是将告警规则转换为timescalesql数据库的查询语句的方法，返回构建告警需要的数据，比如当前哪些节点符合告警规则的条件
一个是将查询到的数据转换为告警对象，这个告警对象的status还不用判断
同时告警规则类会依赖一个数据库的语句执行接口，可以先设计一个抽象接口类

（5）告警规则存储类
告警规则需要存储在postgresql数据库中，先创建一个sql文件用来创建数据表
然后实现告警规则存储类，提供告警规则的存储和获取功能

（6）告警存储类
告警规则评估后生成的告警，需要存储在postgresql数据库中，先创建一个sql文件用来创建数据表
然后实现告警存储类，提供告警的存储和获取功能

（7）更新数据库中的告警状态
告警类应该提供一个更新数据库的方法，这个方法会先从告警存储类获取当前数据库中告警指纹所对应的最新告警
然后数据库中没有这个告警指纹，则在数据库中增加这个告警
如果数据库中已经有这个告警指纹，则先判断告警的类型，再选择是增加还是更新告警

（8）告警引擎类
初始化时，先从数据库中获取所有告警规则并保存在内存中，
然后每隔一段时间（5秒）循环去调用一下告警规则的评估方法，得到当前的全量告警
当前全量告警跟数据库中现有的firing告警做比较，如果数据库中的告警不在当前全量告警了，就可以更新数据库中现有的firing告警的状态
最后再执行全量告警的更新数据库的方法
等待下一轮循环

（9）更新数据库的告警规则
告警引擎类中增加一些方法，用来更新数据库中的告警规则，可以依赖告警规则存储类
当数据库中的告警规则被更新时，内存中的告警规则也要更新

（10）获取告警
告警引擎类中增加一些方法，用来获取数据库中的告警，可以依赖告警存储类

（11）给告警规则增加enabled字段 ✅ 已完成
告警规则类增加enabled字段 true,false
数据库的数据表也要增加字段 enabled true,false，修改sql
告警规则存储类也要相应修改，增加获取enabled规则的方法
告警引擎类获取告警规则时，需要获取enabled true的告警，只有enabled true告警规则需要评估

（12）增加获取告警数量的方法
在告警存储类和告警引擎中增加相关的方法

（13）上报告警的方法

（13）修改web层





