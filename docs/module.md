系统架构

部件组成：
    资源监控部件：
        监控数据管理单元：MonitorManager
            业务流程1：初始化流程
                步骤1：配置HTTP服务并启用CORS
                步骤2：启动组播扫描器（从配置文件读取参数，定期广播管理节点信息）
                步骤3：初始化ResourceRepository（连接TimescaleDB）
                步骤4：初始化MonitorCache（内存缓存）
                步骤5：注册POST /resource路由

            业务流程2：资源数据接收流程
                步骤1：HTTP POST /resource接口接收节点上报的资源数据
                步骤2：解析JSON请求体，提取Resource对象
                步骤3：获取当前秒级时间戳
                步骤4：调用MonitorCache写入内存缓存
                步骤5：调用ResourceRepository持久化到TimescaleDB
                步骤6：返回HTTP 200状态码

            接口1：POST /resource（数据接收接口）
                输入数据：HTTP请求体包含data字段，内含Resource对象的JSON
                处理逻辑：解析JSON、写入MonitorCache、持久化到ResourceRepository
                输出数据：HTTP 200状态码

            接口2：getNodeResource
                输入数据：host_ip（节点IP地址）
                处理逻辑：从MonitorCache查询指定节点的最新资源快照
                输出数据：Resource对象（包含CPU、内存、网络、磁盘、GPU等完整资源信息）

            接口3：queryMetricsSeries
                输入数据：host_ip（节点IP）、duration（时间区间，如"1h"/"5m"）、kinds（资源类型列表）
                处理逻辑：规范化时间区间格式，委托给ResourceRepository查询时序数据
                输出数据：MetricsSeries对象（包含各类资源的时序数据点）

        监控数据存储单元：ResourceRepository
            业务流程1：资源数据持久化流程
                步骤1：建立数据库连接并开启事务
                步骤2：插入心跳记录到resource_alive表
                步骤3：插入CPU数据到resource_cpu表
                步骤4：插入内存数据到resource_memory表
                步骤5：遍历网卡列表，批量插入到resource_network表
                步骤6：遍历磁盘列表，批量插入到resource_disk表
                步骤7：遍历GPU列表，批量插入到resource_gpu表
                步骤8：遍历组件列表，批量插入到component_resource表
                步骤9：提交事务确保原子性

            业务流程2：时序数据查询流程
                步骤1：解析查询参数（host_ip、duration、kinds）
                步骤2：根据kinds分别查询不同资源类型
                步骤3：使用time_bucket_gapfill进行10秒聚合填补空值
                步骤4：对于多维度资源（网络、磁盘、GPU）使用CTE和CROSS JOIN生成完整时间桶
                步骤5：将查询结果转换为对应的Point结构体
                步骤6：删除每个序列的首尾数据点（边界数据可能不完整）
                步骤7：返回MetricsSeries对象

            接口1：save
                输入数据：Resource对象（完整的节点资源信息）
                处理逻辑：单事务内完成多表写入（7张表）
                输出数据：无（异常时抛出）

            接口2：queryMetricsSeries
                输入数据：host_ip、duration（PostgreSQL interval格式）、kinds（资源类型列表）
                处理逻辑：按资源类型分别执行TimescaleDB时序查询，聚合并填补空缺
                输出数据：MetricsSeries对象（包含cpu/memory/network/disk/gpu的时序数据）

        BMC数据管理单元：BMCListener
            业务流程1：初始化和启动流程
                步骤1：保存配置参数（监听IP、组播地址、组播端口）
                步骤2：初始化BMCRepository和BMCCache
                步骤3：创建UDP套接字
                步骤4：设置SO_REUSEADDR和SO_RCVTIMEO选项
                步骤5：绑定端口（INADDR_ANY）
                步骤6：加入组播组（IP_ADD_MEMBERSHIP）
                步骤7：启动监听线程

            业务流程2：BMC数据接收循环
                步骤1：recvfrom接收UDP数据包（1秒超时）
                步骤2：校验数据包长度（>= sizeof(UdpInfo)）
                步骤3：校验包头包尾（0xA55A）
                步骤4：调用用户注册的handler回调（如有）
                步骤5：更新BMCCache缓存（按box_id）
                步骤6：调用BMCRepository持久化到数据库
                步骤7：捕获异常记录日志但不中断循环
                步骤8：检查running标志，继续循环或退出

            接口1：UDP组播接收（数据接收接口）
                输入数据：UDP组播数据包（UdpInfo结构体，包含机箱BMC传感器数据）
                处理逻辑：校验包头包尾、更新BMCCache、持久化到BMCRepository、调用handler回调
                输出数据：无（异步处理）

            接口2：getBoxBMC
                输入数据：box_id（机箱ID）
                处理逻辑：从BMCCache查询指定机箱的最新BMC数据
                输出数据：optional<UdpInfo>（BMC完整报文）

            接口3：getAllBoxBMC
                输入数据：无
                处理逻辑：从BMCCache获取所有机箱的最新BMC数据
                输出数据：vector<UdpInfo>

            接口4：queryBMCSensor
                输入数据：host_ip、duration（时间区间字符串）
                处理逻辑：规范化时间格式，委托给BMCRepository查询传感器时序数据
                输出数据：map<sensorname, vector<BMCSensorRow>>（按传感器名称分组的时序数据）

        BMC数据存储单元：BMCRepository
            业务流程1：BMC数据持久化流程
                步骤1：建立数据库连接并开启事务
                步骤2：遍历2个风扇，插入到bmc_fan表
                步骤3：遍历14块板卡，每块最多5个传感器
                步骤4：跳过空传感器记录（全部字段为0）
                步骤5：IPMB地址映射为槽位ID
                步骤6：根据box_id和slot_id计算host_ip
                步骤7：字节数组转字符串（传感器名称）
                步骤8：插入到bmc_sensor表
                步骤9：提交事务

            业务流程2：传感器时序查询流程
                步骤1：解析查询参数
                步骤2：使用CTE生成时间桶序列和传感器维度
                步骤3：CROSS JOIN生成完整组合
                步骤4：LEFT JOIN实际数据并聚合
                步骤5：按传感器名称分组返回
                步骤6：组合sensorvalue_H和sensorvalue_L为浮点值
                步骤7：删除每个序列的首尾数据点

            接口1：save
                输入数据：UdpInfo结构（BMC组播报文）
                处理逻辑：解析报文，写入bmc_fan和bmc_sensor两张表
                输出数据：无（异常时抛出）

            接口2：queryBMCSensor
                输入数据：host_ip、duration（PostgreSQL interval格式）
                处理逻辑：按传感器维度查询时序数据，10秒聚合
                输出数据：map<sensorname, vector<BMCSensorRow>>

    资源管理部件：
        资源数据管理单元：NodeManager
            业务流程1：初始化流程
                步骤1：接收HttpService依赖注入
                步骤2：创建NodeCache实例
                步骤3：启用CORS跨域支持
                步骤4：启动MulticastScanner（从配置读取参数）
                步骤5：注册POST /heartbeat路由

            业务流程2：心跳接收流程
                步骤1：HTTP POST /heartbeat接口接收节点心跳
                步骤2：解析JSON请求体中的data字段
                步骤3：反序列化为Node对象（17个字段）
                步骤4：调用NodeCache添加或更新节点
                步骤5：返回HTTP 200状态码

            业务流程3：节点查询流程
                步骤1：从NodeCache获取节点数据
                步骤2：获取当前毫秒级时间戳
                步骤3：计算在线状态（距离最后心跳<=10秒为在线）
                步骤4：填充NodeExt的status字段
                步骤5：返回结果

            接口1：POST /heartbeat（数据接收接口）
                输入数据：HTTP请求体包含data字段，内含Node对象的JSON
                处理逻辑：解析JSON、反序列化为Node对象、更新到NodeCache
                输出数据：HTTP 200状态码

            接口2：getAllNodes
                输入数据：无
                处理逻辑：从NodeCache获取所有节点，计算在线状态（10秒阈值）
                输出数据：vector<NodeExt>（包含节点信息、更新时间、在线状态）

            接口3：getNodeByIP
                输入数据：ip（节点IP地址）
                处理逻辑：从NodeCache查询单个节点，计算在线状态
                输出数据：optional<NodeExt>

        资源数据存储单元：NodeCache
            业务流程1：节点添加/更新流程
                步骤1：校验节点IP地址非空
                步骤2：加锁保护并发访问
                步骤3：获取当前毫秒级时间戳
                步骤4：在map中查找或创建NodeRecord
                步骤5：更新node数据和last_update_ms时间戳
                步骤6：返回操作结果

            业务流程2：节点查询流程
                步骤1：校验IP地址非空
                步骤2：加锁保护
                步骤3：在map中查找NodeRecord
                步骤4：构造NodeExt对象（Node + updated_at）
                步骤5：返回optional结果

            接口1：addOrUpdateNode
                输入数据：Node对象（节点完整信息）
                处理逻辑：更新或插入节点到内存map，自动更新时间戳
                输出数据：bool（成功true，失败false）

            接口2：getNode
                输入数据：ip（节点IP地址）
                处理逻辑：从内存map查询节点
                输出数据：optional<NodeExt>

            接口3：getAllNodes
                输入数据：无
                处理逻辑：遍历内存map，构造NodeExt列表
                输出数据：vector<NodeExt>

    告警管理部件：
        告警数据管理单元：AlertManager
            业务流程1：初始化流程
                步骤1：建立数据库连接（从配置读取连接串）
                步骤2：创建规则存储（DatabaseRuleRepository）
                步骤3：创建状态缓存（MemoryAlertRepository）
                步骤4：创建事件存储（DatabaseEventRepository）
                步骤5：创建指纹生成器、时序提供者、评估器、状态管理器
                步骤6：启动调度器（BasicScheduler）
                步骤7：从数据库加载所有启用的规则
                步骤8：为每条规则注册定时任务到调度器

            业务流程2：规则评估定时任务
                步骤1：调度器按eval_every间隔触发任务
                步骤2：调用evaluator评估规则（查询时序数据）
                步骤3：调用state_manager应用状态转换
                步骤4：生成AlertEvent事件列表
                步骤5：通过dispatcher分发事件给内部订阅者
                步骤6：调用push_cb_推送事件到WebSocket客户端

            业务流程3：规则更新流程
                步骤1：从调度器注销旧任务
                步骤2：调用rule_repo_持久化规则到数据库
                步骤3：如果规则启用，注册新的定时任务
                步骤4：设置任务回调（评估→状态转换→事件分发）

            接口1：listRules
                输入数据：无
                处理逻辑：委托给规则存储单元查询所有规则
                输出数据：vector<Rule>

            接口2：upsertRule
                输入数据：Rule对象（规则完整信息）
                处理逻辑：注销旧任务、持久化规则、注册新任务
                输出数据：bool（成功true，失败false）

            接口3：queryEvents
                输入数据：duration（时间区间字符串，如"1h"）
                处理逻辑：委托给事件存储单元查询
                输出数据：vector<AlertEvent>

            接口4：ackAlert
                输入数据：fingerprint（告警指纹）、user（用户名）、comment（备注）
                处理逻辑：委托给状态管理器标记告警已确认
                输出数据：bool

        告警引擎服务单元：AlertService
        
            1. 初始化阶段
            AlertManager.构造函数()
                ├─ 创建所有服务实例
                ├─ scheduler_.start()  启动调度器
                └─ 为每条规则注册任务
            2. 运行阶段（每30秒循环）
            BasicScheduler.触发任务()
                │
                ├─ 第1步：评估规则
                │   BasicAlertEvaluator.evaluate(rule)
                │       └─> SimpleTimeseriesProvider.evaluate(表达式)
                │           └─> 查询TimescaleDB
                │           └─> 返回 [{labels, value}]
                │       └─> 判断：value > threshold ?
                │       └─> 返回 [{labels, matched:true, value, context}]
                │
                ├─ 第2步：状态转换
                │   BasicAlertStateManager.apply(rule, points)
                │       ├─> SimpleFingerprintGenerator.generate()  生成指纹
                │       ├─> MemoryAlertRepository.getState()       查询状态
                │       ├─> 执行状态机转换
                │       ├─> DatabaseEventRepository.append()        持久化事件
                │       ├─> MemoryAlertRepository.upsertState()     更新状态
                │       └─> 返回 [AlertEvent]
                │
                └─ 第3步：事件分发
                    AlertManager
                        ├─> dispatcher_.dispatch(event)  内部订阅者
                        └─> push_cb_(event)              WebSocket推送

            业务流程1：任务调度流程
                步骤1：启动独立工作线程
                步骤2：100ms轮询周期检查所有任务
                步骤3：判断当前时间是否达到任务执行时间
                步骤4：执行任务回调（捕获异常防止影响其他任务）
                步骤5：更新下次执行时间
                步骤6：检查运行标志，继续循环或退出

            业务流程2：时序数据查询流程
                步骤1：解析告警表达式为domain.field.agg格式
                步骤2：根据domain映射到对应的数据库表（cpu/memory/disk/network/gpu/alive）
                步骤3：构建SQL查询语句（支持avg/max/min/last聚合）
                步骤4：处理selector标签构建WHERE条件
                步骤5：对多维度资源添加GROUP BY子句（按device/interface/gpu_index分组）
                步骤6：特殊处理alive心跳检测（使用LEFT JOIN确保无数据时返回0）
                步骤7：执行SQL查询
                步骤8：将查询结果格式化为JSON（包含labels、value、samples、last_ts）
                步骤9：返回时序数据结果

            业务流程3：规则评估和匹配流程
                步骤1：解析规则表达式中的条件（支持单一比较和&&连接的区间判断）
                步骤2：提取比较操作符（>=、<=、==、!=、>、<）和阈值
                步骤3：调用时序数据查询流程获取监控数据
                步骤4：遍历查询结果的每一行数据
                步骤5：提取labels维度标签和value指标值
                步骤6：执行条件匹配判断（所有条件必须同时满足）
                步骤7：构建评估点上下文（包含规则信息、选择器、样本数、时间戳等）
                步骤8：将规则的severity、tag、for_times等元数据合并到上下文
                步骤9：返回EvaluationPoint列表（每个点包含labels、matched、value、context）

            业务流程4：告警状态转换和事件生成流程
                步骤1：遍历所有评估点
                步骤2：根据rule_id和labels生成唯一指纹标识
                步骤3：从内存缓存获取或创建AlertState对象
                步骤4：更新AlertState的基础信息（规则ID、严重级别、标签、评估时间）
                步骤5：判断评估点是否匹配阈值
                步骤6：如果匹配：增加occurrences计数器
                步骤7：根据当前状态和计数执行状态转换（Inactive→Pending→Firing）
                步骤8：如果不匹配：重置occurrences为0，将Firing/Pending转换为Resolved
                步骤9：状态变化时生成AlertEvent事件
                步骤10：将事件写入DatabaseEventRepository持久化
                步骤11：更新AlertState到MemoryAlertRepository缓存
                步骤12：返回所有生成的事件列表

            接口1：evaluate（时序数据查询）
                输入数据：expression（告警表达式）、selector（标签选择器）、window（时间窗口）
                处理逻辑：解析表达式，构建并执行时序数据库查询
                输出数据：JSON对象（包含rows数组，每行含labels、value、samples、last_ts）

            接口2：evaluate（规则评估）
                输入数据：Rule对象（包含表达式、选择器、窗口等）、now_ms（当前时间戳）
                处理逻辑：查询时序数据，根据阈值条件判断是否匹配
                输出数据：vector<EvaluationPoint>（评估点列表）

            接口3：apply（状态转换）
                输入数据：Rule对象、vector<EvaluationPoint>（评估点列表）、now_ms（当前时间戳）
                处理逻辑：执行告警状态机转换，生成告警事件
                输出数据：vector<AlertEvent>（新产生的事件列表）

            接口4：registerTask（注册任务）
                输入数据：task_id（任务ID）、interval_ms（执行间隔）、callback（回调函数）
                处理逻辑：将任务添加到调度器并计算首次执行时间
                输出数据：无

            接口5：ack（确认告警）
                输入数据：fingerprint（告警指纹）、user（确认人）、comment（备注）
                处理逻辑：更新AlertState的acked标志和确认信息
                输出数据：bool（操作是否成功）

        告警规则存储单元：DatabaseRuleRepository
            业务流程1：表初始化流程
                步骤1：检查数据库连接
                步骤2：执行CREATE TABLE IF NOT EXISTS创建alert_rule表
                步骤3：创建enabled字段索引（支持按启用状态查询）
                步骤4：创建severity字段索引（支持按严重级别查询）

            业务流程2：规则查询流程
                步骤1：执行SELECT查询获取规则记录
                步骤2：遍历结果集，逐行解析
                步骤3：转换severity字符串为枚举（"提示"/"一般"/"严重"）
                步骤4：解析JSONB selector字段为LabelSet
                步骤5：构造Rule对象
                步骤6：返回规则列表或单个规则

            业务流程3：规则写入流程
                步骤1：将Severity枚举转换为中文字符串
                步骤2：将LabelSet转换为JSONB字符串
                步骤3：使用INSERT ON CONFLICT DO UPDATE实现UPSERT
                步骤4：更新时自动更新updated_at字段
                步骤5：提交事务

            接口1：listRules
                输入数据：无
                处理逻辑：查询所有规则并按创建时间倒序
                输出数据：vector<Rule>

            接口2：upsertRule
                输入数据：Rule对象
                处理逻辑：使用UPSERT语法插入或更新规则
                输出数据：bool

            接口3：deleteRule
                输入数据：id（规则ID）
                处理逻辑：DELETE语句删除规则
                输出数据：bool（是否实际删除）

        告警事件存储单元：DatabaseEventRepository
            业务流程1：事件追加流程
                步骤1：判断action类型
                步骤2：如果是resolved：更新最近一条firing记录的resolved_time
                步骤3：如果是firing：插入新记录到alert_event表
                步骤4：如果是pending：静默忽略不持久化
                步骤5：转换枚举为字符串（severity、status）
                步骤6：转换时间戳（毫秒→秒）
                步骤7：序列化labels和context为JSON字符串
                步骤8：提交事务

            业务流程2：事件查询流程
                步骤1：解析duration为PostgreSQL INTERVAL
                步骤2：构建SQL查询（WHERE time > now() - interval）
                步骤3：执行查询并按时间倒序
                步骤4：遍历结果集，逐行解析
                步骤5：转换字符串为枚举（severity、status）
                步骤6：反序列化JSON为labels和context对象
                步骤7：构造AlertEvent对象列表

            接口1：append
                输入数据：AlertEvent对象
                处理逻辑：根据action类型选择INSERT或UPDATE操作
                输出数据：bool

            接口2：query
                输入数据：duration（时间区间字符串）
                处理逻辑：查询指定时间范围内的事件
                输出数据：vector<AlertEvent>

            接口3：countByStatus
                输入数据：status（告警状态枚举）
                处理逻辑：COUNT查询统计指定状态的事件数量
                输出数据：size_t（事件总数）