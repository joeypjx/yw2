基于对源代码和头文件的详细分析，该项目按照三个核心模块进行了良好的结构化设计：

  一、资源监控模块 (Monitor Module)

  核心单元

  1. MonitorManager (src/monitor/monitor_manager.cpp)
    - 功能：资源监控的主管理器，处理资源数据收集和HTTP路由
    - 接口：实现 IMonitorModule 接口
  2. MonitorCache (src/monitor/monitor_cache.cpp)
    - 功能：内存缓存，存储最新的资源快照数据
    - 接口：提供资源数据的快速查询
  3. ResourceRepository (src/monitor/resource_repository.cpp)
    - 功能：资源数据持久化，写入PostgreSQL数据库
    - 接口：支持时间序列查询和聚合分析
  4. MulticastScanner (src/utils/MulticastScanner.cpp)
    - 功能：组播扫描器，主动发现和收集节点资源数据
    - 接口：配置化的网络扫描服务

  数据模型

  - Resource (include/yw/monitor_model.h): 完整的资源快照结构
  - NodeResource: 节点级资源(CPU、内存、网络、磁盘、GPU)
  - ComponentEntry: 组件级资源(容器、进程等)
  - MetricsSeries: 时间序列指标数据

  二、资源管理模块 (Node + BMC Module)

  节点管理单元

  1. NodeManager (src/node/node_manager.cpp)
    - 功能：节点生命周期管理，心跳检测
    - 接口：实现 INodeModule，提供节点查询服务
  2. NodeCache (src/node/node_cache.cpp)
    - 功能：节点信息缓存，维护在线/离线状态
    - 接口：高效的节点状态查询

  BMC管理单元

  1. BMCListener (src/bmc/bmc_listener.cpp)
    - 功能：监听BMC组播数据包，处理硬件传感器信息
    - 接口：UDP组播接收和数据解析
  2. BMCCache (src/bmc/bmc_cache.cpp)
    - 功能：BMC数据缓存，按box_id索引
    - 接口：提供BMC传感器数据查询
  3. BMCRepository (src/bmc/bmc_repository.cpp)
    - 功能：BMC数据持久化存储
    - 接口：传感器历史数据查询

  数据模型

  - Node/NodeExt (include/yw/node_model.h): 节点基础信息和扩展信息
  - UdpInfo (include/yw/bmc_model.h): BMC原始数据包结构
  - BMCSensorRow: 传感器时间序列数据

  三、告警管理模块 (Alert Module)

  核心单元

  1. AlertManager (src/alert/AlertManager.cpp)
    - 功能：告警引擎核心，协调各个组件
    - 接口：实现 IAlertModule，提供完整告警管理
  2. AlertServices (src/alert/AlertServices.cpp)
    - 功能：告警服务组件集合
    - 单元：
        - BasicAlertEvaluator: 规则评估器
      - BasicAlertStateManager: 状态管理器
      - SimpleTimeseriesProvider: 时间序列查询
      - BasicScheduler: 任务调度器
  3. DatabaseRuleRepository (src/alert/DatabaseRuleRepository.cpp)
    - 功能：告警规则的数据库存储
    - 接口：规则CRUD操作
  4. DatabaseEventRepository (src/alert/DatabaseEventRepository.cpp)
    - 功能：告警事件的数据库存储
    - 接口：事件查询和统计

  数据模型

  - Rule (include/yw/alert_model.h): 告警规则定义
  - AlertState: 告警状态跟踪
  - AlertEvent: 告警事件记录
  - AlertStatus/Severity: 状态和严重级别枚举

  四、模块间接口关系

  依赖关系图

  AppContext (核心容器)
      ├── NodeModule (节点管理)
      ├── MonitorModule (资源监控) → 依赖 NodeModule
      ├── BMCModule (BMC管理)
      ├── AlertModule (告警管理) → 查询 MonitorModule 数据
      └── WebModule (Web接口) → 聚合所有模块

  关键接口

  1. 资源监控 ← 节点管理
    - MonitorManager 依赖 INodeModule 获取节点列表
    - 通过 MulticastScanner 主动拉取节点资源数据
  2. 告警管理 ← 资源监控
    - SimpleTimeseriesProvider 查询监控数据库
    - 基于资源指标评估告警规则
  3. Web层 ← 所有模块
    - WebFactory::getWebModule() 聚合所有业务模块
    - 提供统一的REST API和WebSocket推送
  4. 数据流向
    - 节点 → HTTP POST /heartbeat → NodeCache
    - 节点 → HTTP POST /resource → MonitorCache + DB
    - BMC → UDP组播 → BMCListener → BMCCache + DB
    - 定时任务 → AlertEvaluator → 查询Monitor DB → 生成AlertEvent

  统一特性

  - 工厂模式: 每个模块都有对应的Factory类进行实例化
  - 接口抽象: 所有模块都定义了Interface层，便于测试和扩展
  - 配置驱动: 通过 JsonConfig 统一管理配置
  - 异步处理: 使用独立线程处理网络监听和定时任务
  - 数据持久化: 统一使用PostgreSQL存储时间序列数据

  这种模块化设计实现了高内聚、低耦合，各模块职责清晰，便于维护和扩展。


  ⏺ 我来详细分析每个单元的内部处理逻辑。

⏺ 一、资源监控模块各单元内部处理逻辑

  1. MonitorManager (src/monitor/monitor_manager.cpp)

  初始化逻辑 (构造函数):
  // 1. 配置HTTP服务CORS
  service_->AllowCORS();

  // 2. 启动资源扫描器
  scanner_ = std::make_unique<yw::utils::MulticastScanner>(
      manager_ip, manager_port, "/resource",
      multicast_ip, multicast_port, interval_ms
  );
  scanner_->start();

  // 3. 初始化数据库仓库
  repository_ = std::make_unique<ResourceRepository>(conninfo);

  // 4. 初始化内存缓存
  monitor_cache_ = std::make_unique<MonitorCache>();

  // 5. 设置HTTP路由
  setupRoutes();

  核心处理逻辑:
  - POST /resource路由: monitor_manager.cpp:97-122
    a. 解析JSON请求体，提取data字段
    b. 反序列化为Resource对象
    c. 获取当前时间戳
    d. 写入内存缓存 (monitor_cache_->put())
    e. 异步写入数据库 (repository_->save())
    f. 错误处理：数据库写入失败时记录日志但不影响缓存

  查询逻辑:
  - getNodeResource(): 从内存缓存获取最新资源快照
  - queryMetricsSeries():
    a. 规范化duration参数 (1h→1 hours, 5m→5 minutes)
    b. 委托给ResourceRepository进行时序查询

  2. MonitorCache (src/monitor/monitor_cache.cpp)

  内部数据结构:
  struct CacheEntry {
      Resource data;
      std::int64_t updated_at_ms;
  };
  std::unordered_map<std::string, CacheEntry> map_; // key=host_ip
  mutable std::mutex mutex_;

  处理逻辑:
  - put(): monitor_cache.cpp:9-14
    a. 线程安全加锁
    b. 按host_ip作为key存储Resource + 时间戳
  - get(): monitor_cache.cpp:16-21
    a. 线程安全查找
    b. 返回optional，未找到返回nullopt
  - getAllHosts(): 遍历map返回所有host_ip列表

  3. ResourceRepository (src/monitor/resource_repository.cpp)

  数据库写入逻辑 save(): resource_repository.cpp:13-105
  pqxx::connection c(conninfo_);
  pqxx::work tx{c};

  // 1. 写入心跳存活表
  tx.exec_params("INSERT INTO resource_alive(time, host_ip, alive) VALUES (now(), $1::inet, 1)", host_ip);

  // 2. 写入CPU指标
  tx.exec_params("INSERT INTO resource_cpu(...) VALUES (now(), $1, $2, ...)",
      host_ip, cpu.usage_percent, cpu.load_avg_1m, ...);

  // 3. 写入内存指标
  tx.exec_params("INSERT INTO resource_memory(...) VALUES (...)", ...);

  // 4. 批量写入网络接口指标
  for (const auto& nic : data.resource.network) {
      tx.exec_params("INSERT INTO resource_network(...) VALUES (...)", ...);
  }

  // 5. 批量写入磁盘分区指标
  for (const auto& disk : data.resource.disk) {
      tx.exec_params("INSERT INTO resource_disk(...) VALUES (...)", ...);
  }

  // 6. 批量写入GPU指标
  for (const auto& gpu : data.resource.gpu) {
      tx.exec_params("INSERT INTO resource_gpu(...) VALUES (...)", ...);
  }

  // 7. 批量写入组件资源
  for (const auto& comp : data.component) {
      tx.exec_params("INSERT INTO component_resource(...) VALUES (...)", ...);
  }

  tx.commit();

  时序查询逻辑 queryMetricsSeries(): resource_repository.cpp:107-406

  CPU查询 (resource_repository.cpp:118-174):
  -- 内层查询：时间分桶+聚合
  SELECT time_bucket_gapfill('10 seconds', time, now() - interval, now()) AS bucket,
         AVG(usage_percent), AVG(load_avg_1m), ...
  FROM resource_cpu
  WHERE host_ip = $1 AND time >= now() - interval
  GROUP BY bucket

  -- 外层查询：格式化+排序
  SELECT EXTRACT(EPOCH FROM bucket)::bigint AS ts,
         COALESCE(usage_percent, 0), ...
  FROM (...) ORDER BY ts ASC

  网络/磁盘/GPU查询 - 维度分组模式:
  -- 1. 生成时间分桶序列
  WITH bucket_series AS (
    SELECT generate_series(time_bucket('10 seconds', now() - interval),
                          time_bucket('10 seconds', now()), '10 seconds') AS bucket
  ),
  -- 2. 获取所有维度值(interface/device/gpu_index)
  dims AS (SELECT DISTINCT interface FROM resource_network WHERE ...)

  -- 3. 笛卡尔积 + LEFT JOIN 填充缺失数据
  SELECT dims.interface, buckets.bucket, COALESCE(AVG(metrics.value), 0)
  FROM dims CROSS JOIN bucket_series AS buckets
  LEFT JOIN resource_network AS metrics ON ...
  GROUP BY dims.interface, buckets.bucket
  ORDER BY dims.interface, ts ASC

  数据清理: 删除首尾数据点避免边界效应

⏺ 二、资源管理模块各单元内部处理逻辑

  1. NodeManager (src/node/node_manager.cpp)

  初始化逻辑:
  // 1. 创建节点缓存
  node_cache_ = std::make_unique<NodeCache>();

  // 2. 启动节点扫描器
  scanner_ = std::make_unique<yw::utils::MulticastScanner>(
      manager_ip, manager_port, "/heartbeat",  // 心跳端点
      multicast_ip, multicast_port, interval_ms
  );
  scanner_->start();

  // 3. 设置HTTP路由
  setupRoutes();

  心跳处理逻辑 (node_manager.cpp:70-81):
  service_->POST("/heartbeat", [this](const HttpContextPtr& ctx) {
      // 1. 解析JSON请求体
      const auto j = nlohmann::json::parse(ctx->body());

      // 2. 提取data字段并转换为Node对象
      if (j.contains("data")) {
          const Node node = j["data"].get<Node>();

          // 3. 更新节点缓存（自动更新时间戳）
          node_cache_->addOrUpdateNode(node);
      }

      return 200; // 直接返回成功
  });

  状态计算逻辑:
  - getAllNodes() (node_manager.cpp:41-51):
    a. 从缓存获取所有节点
    b. 计算当前时间戳
    c. 遍历节点，判断在线状态: (now_ms - updated_at) <= 10000 (10秒超时)
    d. 设置status字段: "online" | "offline"
  - getNodeByIP() (node_manager.cpp:53-61): 类似逻辑，单节点查询

  2. NodeCache (src/node/node_cache.cpp)

⏺ Read(src/node/node_cache.cpp)
  ⎿  Read 57 lines (ctrl+o to expand)

⏺ NodeCache内部数据结构:
  struct NodeRecord {
      Node node;
      std::int64_t last_update_ms;
  };
  std::unordered_map<std::string, NodeRecord> nodes_; // key=host_ip
  mutable std::mutex mutex_;

  处理逻辑:
  - addOrUpdateNode() (node_cache.cpp:13-28):
    a. 验证host_ip非空
    b. 线程安全加锁
    c. 获取当前毫秒时间戳
    d. 更新或插入NodeRecord(node + timestamp)
  - getNode() (node_cache.cpp:30-42):
    a. IP验证
    b. 线程安全查找
    c. 构造NodeExt对象(包含updated_at字段)
  - getAllNodes() (node_cache.cpp:44-53): 遍历map构造NodeExt向量

  3. BMCListener (src/bmc/bmc_listener.cpp)

  UDP组播监听逻辑:

  Socket初始化 (bmc_listener.cpp:50-86):
  // 1. 创建UDP socket
  sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);

  // 2. 设置端口复用
  setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  // 3. 绑定到组播端口
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(mcast_port_);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sock_, &addr, sizeof(addr));

  // 4. 设置接收超时(1秒)
  timeval tv{1, 0};
  setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  // 5. 加入组播组
  ip_mreq mreq{};
  mreq.imr_multiaddr.s_addr = inet_addr(mcast_group_.c_str());
  mreq.imr_interface.s_addr = listen_ip_.empty() ? INADDR_ANY : inet_addr(listen_ip_.c_str());
  setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

  数据包处理循环 (bmc_listener.cpp:97-144):
  while (running_) {
      uint8_t buffer[2048];
      sockaddr_in src{};
      socklen_t slen = sizeof(src);

      // 1. 接收UDP数据包
      ssize_t n = ::recvfrom(sock_, buffer, sizeof(buffer), 0, &src, &slen);

      // 2. 错误处理
      if (n < 0) {
          if (errno == EAGAIN) continue; // 超时，继续检查running_
          if (errno == EINTR) continue;  // 信号中断
          if (errno == EBADF) break;     // socket已关闭
          // 其他错误记录日志后继续
      }

      // 3. 数据包长度验证
      if (n < sizeof(UdpInfo)) {
          spdlog::warn("包长度过小，丢弃");
          continue;
      }

      // 4. 数据包格式验证
      const UdpInfo* pkt = reinterpret_cast<const UdpInfo*>(buffer);
      if (pkt->head != 0xA55A || pkt->tail != 0xA55A) {
          spdlog::warn("包头/包尾校验失败，丢弃");
          continue;
      }

      // 5. 调用用户处理回调
      if (handler_) handler_(*pkt);

      // 6. 写入内存缓存
      if (bmc_cache_) bmc_cache_->addOrUpdate(*pkt);

      // 7. 持久化到数据库
      if (repository_) {
          try {
              repository_->save(*pkt);
          } catch (const std::exception& e) {
              spdlog::error("BMC数据保存失败: {}", e.what());
          }
      }
  }

  4. BMCCache (src/bmc/bmc_cache.cpp)

  内部数据结构:
  struct Record {
      UdpInfo info;
      std::int64_t last_update_ms;
  };
  std::unordered_map<int, Record> cache_; // key=box_id
  mutable std::mutex mutex_;

  处理逻辑:
  - addOrUpdate() (bmc_cache.cpp:7-20):
    a. 从UdpInfo.boxid提取box_id作为key
    b. 线程安全更新缓存记录
  - getByBoxId() (bmc_cache.cpp:22-28): 按box_id查询单个BMC信息
  - getAll() (bmc_cache.cpp:30-38): 返回所有BMC信息列表

⏺ 三、告警管理模块各单元内部处理逻辑

  1. AlertManager (src/alert/AlertManager.cpp)

  初始化逻辑 (AlertManager.cpp:38-76):
  // 1. 创建数据库连接
  conn_ = std::make_shared<pqxx::connection>(conninfo);

  // 2. 组装核心服务组件
  rule_repo_     = std::make_shared<DatabaseRuleRepository>(conn_);      // 规则存储
  alert_repo_    = std::make_shared<MemoryAlertRepository>();           // 告警状态(内存)
  event_repo_    = std::make_shared<DatabaseEventRepository>(conn_);    // 事件存储
  fp_            = std::make_shared<SimpleFingerprintGenerator>();      // 指纹生成
  ts_            = std::make_shared<SimpleTimeseriesProvider>(conn_);   // 时序查询
  evaluator_     = std::make_shared<BasicAlertEvaluator>(ts_);         // 规则评估
  state_manager_ = std::make_shared<BasicAlertStateManager>(alert_repo_, event_repo_, fp_);
  scheduler_     = std::make_shared<BasicScheduler>();                  // 任务调度

  // 3. 启动调度器
  scheduler_->start();

  // 4. 为每条启用的规则注册定时任务
  for (const auto& rule : rule_repo_->listRules()) {
      if (!rule.enabled) continue;

      const auto interval_ms = parseDurationMs(rule.eval_every, 30000); // 默认30秒
      scheduler_->registerTask(rule.id, interval_ms, [this, rule]{
          // 评估-应用-推送 流水线
          const auto now_ms = getCurrentTimestamp();
          auto points = evaluator_->evaluate(rule, now_ms);        // 规则评估
          auto events = state_manager_->apply(rule, points, now_ms); // 状态转换

          // 事件分发
          for (const auto& ev : events) {
              dispatcher_.dispatch(AlertManagerEvent::AlertEventAppended, ev);
              if (push_cb_) push_cb_(ev); // Web层推送回调
          }
      });
  }

  规则管理逻辑:
  - upsertRule() (AlertManager.cpp:85-102):
    a. 先注销旧的定时任务: scheduler_->unregisterTask(rule.id)
    b. 持久化规则: rule_repo_->upsertRule(rule)
    c. 如果规则启用，重新注册定时任务
  - deleteRule() (AlertManager.cpp:103-106):
    a. 注销定时任务
    b. 删除数据库记录

  2. BasicAlertEvaluator (src/alert/AlertServices.cpp)

  规则评估主流程 (AlertServices.cpp:411-495):
  std::vector<EvaluationPoint> evaluate(const Rule& rule, std::int64_t now_ms) {
      // 1. 解析表达式条件
      const auto parsed_conditions = parse_conditions(rule.expression);

      // 2. 调用时序提供者获取数据
      nlohmann::json res = ts_->evaluate(rule.expression, rule.selector, rule.window);

      // 3. 遍历时序数据行
      for (const auto& row : res["rows"]) {
          EvaluationPoint point;

          // 4. 提取标签
          if (row.contains("labels")) {
              for (auto it = row["labels"].begin(); it != row["labels"].end(); ++it) {
                  point.labels[it.key()] = convert_to_string(it.value());
              }
          }

          // 5. 提取数值
          point.value = row.contains("value") ? row["value"].get<double>() : 0.0;

          // 6. 计算匹配状态
          if (row.contains("matched")) {
              point.matched = row["matched"].get<bool>(); // 优先使用provider结果
          } else if (parsed_conditions.ok) {
              // 本地条件评估
              bool all_match = true;
              for (const auto& cond : parsed_conditions.conds) {
                  if (!eval_match(point.value, cond.op, cond.threshold)) {
                      all_match = false;
                      break;
                  }
              }
              point.matched = all_match;
          }

          // 7. 构建上下文信息
          point.context["rule_id"] = rule.id;
          point.context["rule_name"] = rule.name;
          point.context["severity"] = severity_to_string(rule.severity);
          // 合并rule.selector到context
          for (const auto& [k, v] : rule.selector) {
              point.context[k] = v;
          }
      }
  }

  表达式解析逻辑 (AlertServices.cpp:334-408):
  static ParsedConds parse_conditions(const std::string& expr) {
      // 1. 按"&&"分割条件
      std::vector<std::string> parts = split_by_and(expr);

      // 2. 解析每个单一条件
      for (const auto& part : parts) {
          // 支持操作符: >=, <=, ==, !=, >, <
          for (const auto& op : {">=", "<=", "==", "!=", ">", "<"}) {
              auto pos = part.find(op);
              if (pos != std::string::npos) {
                  // 提取右侧阈值
                  double threshold = std::stod(extract_number_after_op(part, pos, op));
                  conditions.push_back({op, threshold});
                  break;
              }
          }
      }

      return conditions;
  }

  3. SimpleTimeseriesProvider (src/alert/AlertServices.cpp)

  时序数据查询逻辑 (AlertServices.cpp:26-248):
  nlohmann::json evaluate(const std::string& expression, const LabelSet& selector, const std::string& window) {
      // 1. 解析metric表达式: domain.field.agg
      auto tokens = split_by_dot(expression_left_side);
      std::string domain = tokens[0]; // cpu/memory/disk/network/gpu/alive
      std::string field = tokens[1];  // usage_percent/total/rx_bytes...
      std::string agg = tokens.size() >= 3 ? tokens[2] : "avg"; // avg/max/min/last

      // 2. 映射到数据库表
      std::string table, value_col;
      std::vector<std::string> partition_cols;

      if (domain == "cpu") {
          table = "resource_cpu"; value_col = field;
      } else if (domain == "memory") {
          table = "resource_memory"; value_col = field;
      } else if (domain == "disk") {
          table = "resource_disk"; value_col = field;
          partition_cols = {"device", "mount_point"}; // 磁盘分区维度
      } else if (domain == "network") {
          table = "resource_network"; value_col = field;
          partition_cols = {"interface"}; // 网卡接口维度
      } else if (domain == "gpu") {
          table = "resource_gpu"; value_col = field;
          partition_cols = {"gpu_index"}; // GPU索引维度
      } else if (domain == "alive") {
          // 特殊处理：心跳存活检测
          return query_alive_status(selector, window);
      }

      // 3. 构建SQL查询
      std::string sql;
      if (agg == "last" || agg == "latest") {
          // 最新值查询
          sql = "SELECT DISTINCT ON (" + group_by + ") " + select_cols + ", " + value_col +
                " FROM " + table + " WHERE time > now() - " + interval +
                " ORDER BY " + group_by + ", time DESC";
      } else {
          // 聚合查询
          std::string agg_fn = (agg == "max") ? "MAX" : (agg == "min") ? "MIN" : "AVG";
          sql = "SELECT " + select_cols + ", " + agg_fn + "(" + value_col + ") AS value" +
                " FROM " + table + " WHERE time > now() - " + interval +
                " GROUP BY " + group_by;
      }

      // 4. 执行查询并转换为JSON格式
      pqxx::result r = tx.exec_params(sql, selector_params...);
      for (const auto& row : r) {
          nlohmann::json item;
          item["labels"]["host_ip"] = row["host_ip"].as<std::string>();
          // 添加分区维度标签
          for (const auto& col : partition_cols) {
              item["labels"][col] = row[col].as<std::string>();
          }
          item["value"] = row["value"].as<double>();
          item["samples"] = row["samples"].as<long long>();
          result["rows"].push_back(item);
      }
  }

  心跳存活特殊处理:
  // 窗口内无心跳记录时返回value=0，有记录时返回聚合值
  if (selector.contains("host_ip")) {
      sql = "SELECT host($1::inet) AS host_ip, "
            "COALESCE(MAX(alive), 0) AS value "
            "FROM (SELECT $1::inet AS host_ip) h "
            "LEFT JOIN resource_alive a ON a.host_ip = h.host_ip AND a.time > now() - interval "
            "GROUP BY h.host_ip";
  }

  4. BasicAlertStateManager (src/alert/AlertServices.cpp)

  状态转换逻辑 (AlertServices.cpp:503-567):
  std::vector<AlertEvent> apply(const Rule& rule, const std::vector<EvaluationPoint>& points, std::int64_t now_ms) {
      for (const auto& point : points) {
          // 1. 生成指纹
          auto fingerprint = fp_->generate(rule.id, point.labels);

          // 2. 获取或创建告警状态
          auto state = repo_->getState(fingerprint).value_or(
              AlertState{fingerprint, rule.id}
          );
          state.last_eval_ms = now_ms;

          // 3. 准备事件对象
          AlertEvent event;
          event.timestamp_ms = now_ms;
          event.fingerprint = fingerprint;
          event.rule_id = rule.id;
          event.value = point.value;
          event.context = point.context;

          if (point.matched) {
              // 4a. 条件匹配：状态转换
              state.occurrences++;

              if (state.status == AlertStatus::Inactive || state.status == AlertStatus::Resolved) {
                  // Inactive/Resolved -> Pending
                  state.status = AlertStatus::Pending;
                  state.first_firing_ms = now_ms;
                  state.last_change_ms = now_ms;

                  event.action = "pending";
                  event.status = AlertStatus::Pending;
                  events.push_back(event);

              } else if (state.status == AlertStatus::Pending && state.occurrences >= rule.for_times) {
                  // Pending -> Firing (达到for_times阈值)
                  state.status = AlertStatus::Firing;
                  state.last_change_ms = now_ms;

                  event.action = "firing";
                  event.status = AlertStatus::Firing;
                  events.push_back(event);
              }
              // Firing状态继续更新计数但不产生新事件

          } else {
              // 4b. 条件不匹配：恢复处理
              state.occurrences = 0;

              if (state.status == AlertStatus::Firing || state.status == AlertStatus::Pending) {
                  // Firing/Pending -> Resolved
                  state.status = AlertStatus::Resolved;
                  state.last_change_ms = now_ms;

                  event.action = "resolved";
                  event.status = AlertStatus::Resolved;
                  events.push_back(event);
              }
          }

          // 5. 持久化状态
          repo_->upsertState(state);
      }

      return events;
  }

  5. BasicScheduler (src/alert/AlertServices.cpp)

  任务调度逻辑 (AlertServices.cpp:588-623):
  struct TaskEntry {
      std::int64_t interval_ms;
      Task task;
      std::int64_t next_run_ms;
  };
  std::unordered_map<std::string, TaskEntry> tasks_;

  // 调度循环
  void worker_thread() {
      while (running_) {
          const auto now_ms = getCurrentTimestamp();

          std::lock_guard<std::mutex> lock(mutex_);
          for (auto& [id, task_entry] : tasks_) {
              if (now_ms >= task_entry.next_run_ms) {
                  try {
                      task_entry.task(); // 执行任务
                  } catch (...) {
                      // 忽略任务异常，避免影响其他任务
                  }
                  // 计算下次执行时间
                  task_entry.next_run_ms = now_ms + task_entry.interval_ms;
              }
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms轮询间隔
      }
  }

⏺ 四、核心组件和工具类的处理逻辑

  1. MulticastScanner (src/utils/MulticastScanner.cpp)

  组播发现机制逻辑:

  初始化和启动 (MulticastScanner.cpp:35-48):
  void start() {
      if (running_.exchange(true)) return; // 原子操作防止重复启动

      if (!openSocket()) {
          running_ = false;
          return;
      }

      // 启动工作线程
      worker_ = std::thread(&MulticastScanner::runLoop, this);
  }

  Socket配置 (MulticastScanner.cpp:92-113):
  bool openSocket() {
      // 1. 创建UDP socket
      sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);

      // 2. 设置组播出接口（如果指定了manager_ip）
      if (!manager_ip_.empty()) {
          in_addr ifaddr{};
          ifaddr.s_addr = inet_addr(manager_ip_.c_str());
          setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr));
      }

      // 3. 设置TTL=1（本地网段组播）
      int ttl = 1;
      setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

      return true;
  }

  发现消息发送循环 (MulticastScanner.cpp:54-59):
  void runLoop() {
      while (running_) {
          sendOnce(); // 发送一次组播消息
          std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_)); // 等待间隔
      }
  }

  组播消息格式 (MulticastScanner.cpp:61-90):
  bool sendOnce() {
      // 1. 构建JSON消息
      json payload = {
          {"api_version", 1},
          {"data", {
              {"manager_ip", manager_ip_},    // 管理器IP
              {"manager_port", manager_port_}, // 管理器端口
              {"url", url_}                   // 回调URL (/heartbeat 或 /resource)
          }}
      };

      // 2. 发送到组播地址
      sockaddr_in dest{};
      dest.sin_family = AF_INET;
      dest.sin_port = htons(multicast_port_);          // 默认3980
      dest.sin_addr.s_addr = inet_addr(multicast_ip_.c_str()); // 默认239.192.168.80

      ssize_t n = sendto(sock_, payload_str.data(), payload_str.size(), 0, &dest, sizeof(dest));

      // 3. 网络错误处理
      if (n < 0 && is_network_error(errno)) {
          closeSocket(); // 重新打开socket
      }
  }

  工作原理:
  - 发现阶段: MulticastScanner定时向组播地址发送管理器信息
  - 响应阶段: 节点收到组播消息后，向指定的manager_ip:port/url发送数据
  - 容错机制: 网络异常时自动重建Socket

  2. JsonConfig (src/utils/JsonConfig.cpp)

  单例配置管理逻辑:

  单例模式 (JsonConfig.cpp:8-11):
  JsonConfig& JsonConfig::instance() {
      static JsonConfig cfg; // 线程安全的静态局部变量
      return cfg;
  }

  配置文件加载 (JsonConfig.cpp:13-23):
  bool loadFromFile(const std::string& path) {
      std::ifstream ifs(path);
      if (!ifs.is_open()) return false;

      try {
          nlohmann::json j;
          ifs >> j;           // 流式JSON解析
          data_ = std::move(j); // 原子替换配置数据
          return true;
      } catch (...) {
          data_.clear();      // 解析失败时清空数据
          return false;
      }
  }

  模板化配置获取 (头文件 include/yw/JsonConfig.h):
  template<typename T>
  static T Get(const std::string& key, const T& default_value = T{}) {
      auto& inst = instance();
      std::lock_guard<std::mutex> lock(inst.mutex_);

      // 支持嵌套键访问: "db.host" -> data_["db"]["host"]
      auto keys = split_by_dot(key);
      nlohmann::json* current = &inst.data_;

      for (const auto& k : keys) {
          if (!current->contains(k)) return default_value;
          current = &(*current)[k];
      }

      try {
          return current->get<T>();
      } catch (...) {
          return default_value;
      }
  }

  3. WebController (src/web/web_controller.cpp)

  Web层聚合逻辑:

  初始化和依赖注入 (web_controller.cpp:24-52):
  WebController::WebController(server, service, node_module, monitor_module, bmc_module, alert_module) {
      // 1. 创建WebSocket推送器
      pusher_ = std::make_unique<AlertPusher>(server_.get());

      // 2. 配置HTTP服务
      if (service_) {
          service_->AllowCORS();  // 跨域支持
          setupRoutes();          // 注册所有路由
      }

      // 3. 建立告警推送回调链
      if (alert_module_) {
          alert_module_->setPushCallback([this](const alert::AlertEvent& event){
              if (pusher_) {
                  pusher_->push(event); // 转发到WebSocket客户端
              }
          });
      }
  }

  路由分域注册 (web_controller.cpp:60-66):
  void setupRoutes() {
      // 按功能域分别注册路由，实现关注点分离
      routes::registerNodeRoutes(service_.get(), node_module_.get(), monitor_module_.get());
      routes::registerMetricsRoutes(service_.get(), node_module_.get(), monitor_module_.get(), bmc_module_.get());
      routes::registerBMCRoutes(service_.get(), bmc_module_.get());
      routes::registerAlertRoutes(service_.get(), alert_module_.get(), pusher_.get());
  }


⏺ 4. AlertPusher (src/web/AlertPusher.cpp)

  WebSocket实时推送逻辑:

  WebSocket服务初始化 (AlertPusher.cpp:20-36):
  bool init() {
      // 1. 连接建立处理
      ws_service_->onopen = [this](const WebSocketChannelPtr& channel, const HttpRequestPtr&){
          std::lock_guard<std::mutex> lock(mutex_);
          channels_.insert(channel); // 加入活跃连接集合
      };

      // 2. 连接关闭处理  
      ws_service_->onclose = [this](const WebSocketChannelPtr& channel){
          std::lock_guard<std::mutex> lock(mutex_);
          channels_.erase(channel); // 从活跃连接中移除
      };

      // 3. 消息回显（心跳检测）
      ws_service_->onmessage = [](const WebSocketChannelPtr& channel, const std::string& msg){
          channel->send(msg); // 简单回显
      };

      return true;
  }

  告警事件推送 (AlertPusher.cpp:38-52):
  void push(const alert::AlertEvent& event) {
      // 1. 数据转换：内部事件 -> 用户视图
      auto user_view = mapper::toUserAlertEventView(event);
      nlohmann::json json_payload = user_view;
      std::string message = json_payload.dump();

      // 2. 广播到所有活跃连接
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto it = channels_.begin(); it != channels_.end(); ) {
          auto channel = *it;

          // 3. 连接有效性检查
          if (!channel || !channel->isConnected()) {
              it = channels_.erase(it); // 清理失效连接
              continue;
          }

          // 4. 发送消息
          channel->send(message);
          ++it;
      }
  }

  连接管理:
  - 数据结构: std::unordered_set<WebSocketChannelPtr> channels_
  - 线程安全: 所有操作都有mutex保护
  - 自动清理: 推送时检查连接状态并清理失效连接

  5. NodeRoutes (src/web/routes/NodeRoutes.cpp)

  节点查询路由逻辑:

  GET /node路由处理 (NodeRoutes.cpp:17-87):
  service->GET("/node", [node_module, monitor_module](const HttpContextPtr& ctx) {
      // 1. 获取所有节点
      const auto nodes = node_module->getAllNodes();

      // 2. 解析查询参数
      auto params = ctx->params();
      int filter_box_id = -1;
      bool has_box_filter = false;
      std::string filter_host_ip;
      bool has_host_filter = false;

      // box_id过滤
      if (params.contains("box_id")) {
          try {
              filter_box_id = std::stoi(params["box_id"]);
              has_box_filter = true;
          } catch (...) {
              return error_response("invalid box_id", HTTP_STATUS_BAD_REQUEST);
          }
      }

      // host_ip过滤
      if (params.contains("host_ip")) {
          filter_host_ip = params["host_ip"];
          has_host_filter = true;
      }

      // 3. 数据聚合和过滤
      nlohmann::json response_nodes = nlohmann::json::array();
      for (const auto& node_ext : nodes) {
          // 应用过滤条件
          if (has_box_filter && node_ext.box_id != filter_box_id) continue;
          if (has_host_filter && node_ext.host_ip != filter_host_ip) continue;

          // 获取节点资源信息（可选）
          const monitor::Resource* resource = nullptr;
          if (monitor_module) {
              auto resource_ptr = monitor_module->getNodeResource(node_ext.host_ip);
              if (resource_ptr) {
                  resource = resource_ptr.get();
              }
          }

          // 4. 数据映射：内部模型 -> API视图
          auto node_view = mapper::toNodeView(node_ext, resource);
          response_nodes.push_back(node_view);
      }

      // 5. 响应格式化
      nlohmann::json response;
      if (has_host_filter) {
          // 单节点查询：返回对象或空对象
          response = {
              {"api_version", 1},
              {"data", response_nodes.empty() ? json::object() : response_nodes[0]},
              {"status", "success"}
          };
      } else {
          // 列表查询：返回数组
          response = {
              {"api_version", 1},
              {"data", {{"nodes", response_nodes}}},
              {"status", "success"}
          };
      }

      ctx->setContentType("application/json");
      return ctx->send(response.dump(2));
  });

⏺ Read(src/core/app_context.cpp)
  ⎿  Read 50 lines (ctrl+o to expand)

⏺ 6. AppContext (src/core/app_context.cpp)

  应用上下文管理逻辑:

  初始化流程 (app_context.cpp:16-43):
  bool initialize() {
      std::lock_guard<std::mutex> lock(mutex_);

      // 1. 防重复初始化
      if (http_server_) {
          spdlog::info("AppContext already initialized");
          return true;
      }

      try {
          // 2. 创建HTTP服务器组件
          http_server_ = std::make_shared<hv::HttpServer>();
          http_service_ = std::make_shared<hv::HttpService>();

          // 3. 从配置文件加载服务器参数
          const std::string host = JsonConfig::Get<std::string>("host", "0.0.0.0");
          const int port = JsonConfig::Get<int>("port", 18888);
          const int threads = JsonConfig::Get<int>("thread_num", 4);

          // 4. 配置HTTP服务器
          http_server_->setHost(host.c_str());
          http_server_->setPort(port);
          http_server_->setThreadNum(threads);

          return true;
      } catch (const std::exception& e) {
          spdlog::error("Failed to initialize AppContext: {}", e.what());
          return false;
      }
  }

  HTTP服务器启动 (app_context.cpp:80-102):
  void runHttpServer() {
      std::lock_guard<std::mutex> lock(mutex_);

      if (!http_server_) {
          spdlog::error("HTTP server not initialized");
          return;
      }

      if (http_thread_.joinable()) {
          spdlog::warn("HTTP server thread already running");
          return;
      }

      // 启动HTTP服务器线程
      http_thread_ = std::thread([server = http_server_, service = http_service_]() {
          // 1. 注册共享路由服务
          server->registerHttpService(service.get());

          // 2. 启动服务器（阻塞调用）
          int ret = server->run();
          if (ret != 0) {
              spdlog::error("Failed to start HTTP server, error code: {}", ret);
              return;
          }

          spdlog::info("HTTP server started on {}:{}",
                       JsonConfig::Get<std::string>("host", "0.0.0.0"),
                       JsonConfig::Get<int>("port", 18888));
      });
  }

  模块依赖注入 (app_context.cpp:105-137):
  // 线程安全的模块注入方法
  void setNodeModule(std::shared_ptr<node::INodeModule> m) {
      std::lock_guard<std::mutex> lock(mutex_);
      node_module_ = std::move(m);
  }

  void setMonitorModule(std::shared_ptr<monitor::IMonitorModule> m) {
      std::lock_guard<std::mutex> lock(mutex_);
      monitor_module_ = std::move(m);
  }

  // ... 其他模块注入方法类似

  资源清理 (app_context.cpp:55-78):
  void cleanup() {
      std::lock_guard<std::mutex> lock(mutex_);

      // 1. 先停业务模块（释放依赖）
      node_module_.reset();
      monitor_module_.reset();
      bmc_module_.reset();
      alert_module_.reset();

      if (http_server_) {
          // 2. 停止HTTP服务器
          http_server_->stop();

          // 3. 等待HTTP线程结束
          if (http_thread_.joinable()) {
              http_thread_.join();
          }

          // 4. 重置服务器实例
          http_server_.reset();
          http_service_.reset();

          spdlog::info("HTTP server stopped and AppContext cleaned up");
      }
  }

⏺ 各单元内部处理逻辑总结

  数据流转模式

  1. 推送式数据收集:
  节点Agent → HTTP POST → MonitorManager → MonitorCache + DB
  节点Agent → HTTP POST → NodeManager → NodeCache
  BMC硬件 → UDP组播 → BMCListener → BMCCache + DB

  2. 拉取式服务发现:
  MulticastScanner → 组播通告 → 节点收到 → 回调管理器

  3. 事件驱动告警:
  定时器 → AlertEvaluator → 查询时序DB → 状态转换 → AlertEvent → WebSocket推送

  核心设计模式

  1. 缓存+持久化双写模式:
  - 所有数据先写内存缓存（快速查询）
  - 异步写入数据库（历史查询、聚合分析）
  - 写入失败不影响缓存服务

  2. 状态机模式（告警生命周期）:
  Inactive → Pending → Firing → Resolved
           ↗         ↗        ↘
        匹配1次   达到阈值次数   条件恢复

  3. 观察者模式（事件分发）:
  - AlertManager产生事件 → 推送回调 → WebSocket广播
  - 支持多个观察者注册

  4. 工厂模式（模块创建）:
  - 每个模块都有对应Factory，便于依赖注入和测试

  5. 策略模式（时序查询）:
  - 根据domain（cpu/memory/disk/network/gpu/alive）选择不同查询策略
  - 支持不同聚合函数（avg/max/min/last）

  容错和性能优化

  1. 线程安全:
  - 所有缓存操作都有mutex保护
  - 使用atomic操作控制生命周期状态

  2. 连接管理:
  - Socket自动重连（网络异常时）
  - WebSocket连接自动清理（失效检测）

  3. 数据库优化:
  - 时间分桶聚合（10秒间隔）
  - 时间序列填充（gapfill处理缺失数据）
  - 索引优化（enabled+created_at组合索引）

  4. 错误处理:
  - 数据库操作异常不影响缓存服务
  - JSON解析失败时使用默认值
  - 网络错误时记录日志但继续服务

  这套架构实现了高可用、高性能的资源监控和告警系统，各模块职责清晰，扩展性良好。