# 模块依赖关系文档

本文档描述了 `src` 目录下各个模块之间的依赖关系。

## 模块概览

```
src/
├── app/          # 应用入口，负责模块初始化和启动
├── core/         # 核心模块，提供 AppContext 和基础服务
├── utils/        # 工具模块，提供通用工具函数
├── node/         # 节点模块，管理节点信息和状态
├── monitor/      # 监控模块，处理资源监控数据
├── bmc/          # BMC模块，处理BMC传感器数据
├── controller/   # 控制器模块，提供资源控制功能
├── alert/        # 告警模块，处理告警规则和事件
└── web/          # Web模块，提供HTTP API路由
```

## 编译时依赖关系

### 1. core（核心模块）
- **依赖**：
  - `yw_utils` (PRIVATE) - 使用 JsonConfig 读取配置
  - `spdlog`, `hv_static`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **被依赖**：`app`（使用 AppContext）
- **说明**：提供应用上下文和基础服务。不依赖任何业务模块，只使用前向声明管理模块指针。第三方库依赖通过 `yw_utils` 传递

### 2. utils（工具模块）
- **依赖**：
  - `spdlog`, `nlohmann_json`, `hv_static`, `pqxx` (第三方库，PUBLIC)
- **被依赖**：
  - `core` - 使用 JsonConfig
  - `node` - 使用 MulticastScanner, JsonConfig
  - `monitor` - 使用 MulticastScanner, JsonConfig, DurationUtils, PostgreSQLConnectionPool
  - `controller` - 链接但未直接使用（可能是预留）
  - `bmc` - 使用 JsonConfig, PostgreSQLConnectionPool
  - `alert` - 使用 JsonConfig, IPAddressUtils, TimeUtils, DurationUtils, PostgreSQLConnectionPool
  - `app` - 使用 JsonConfig
  - `web` - 使用 ResponseBuilder
- **说明**：提供通用工具函数和类：
  - `JsonConfig` - JSON配置文件读取
  - `DurationUtils` - 持续时间解析工具
  - `TimeUtils` - ISO时间字符串解析工具
  - `IPAddressUtils` - IP地址与机箱槽位转换工具
  - `PostgreSQLConnectionPool` - PostgreSQL数据库连接池
  - `MulticastScanner` - 组播扫描工具
  - `ResponseBuilder` - HTTP响应构建工具（从 `src/web/utils/` 移动到 `yw_utils` 模块）

### 3. node（节点模块）
- **依赖**：
  - `yw_utils` (PRIVATE)
  - `spdlog`, `hv_static`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **被依赖**：`web`（运行时注入）
- **说明**：管理节点信息和状态，提供节点查询接口

### 4. monitor（监控模块）
- **依赖**：
  - `yw_utils` (PRIVATE) - 使用 JsonConfig, PostgreSQLConnectionPool
  - `pqxx` (PostgreSQL客户端，通过 yw_utils 的 PUBLIC 链接自动传递)
  - `spdlog`, `hv_static`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **被依赖**：`web`（运行时注入）
- **说明**：处理节点资源监控数据，提供资源查询和导出功能。使用 `PostgreSQLConnectionPool` 进行数据库连接管理

### 5. bmc（BMC模块）
- **依赖**：
  - `yw_utils` (PRIVATE) - 使用 JsonConfig, PostgreSQLConnectionPool
  - `pqxx` (PostgreSQL客户端，通过 yw_utils 的 PUBLIC 链接自动传递)
  - `spdlog`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **被依赖**：`web`（运行时注入）
- **说明**：处理BMC传感器数据，提供BMC信息查询接口。使用 `PostgreSQLConnectionPool` 进行数据库连接管理

### 6. controller（控制器模块）
- **依赖**：
  - `yw_utils` (PRIVATE)
  - `spdlog` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **被依赖**：`web`（运行时注入）
- **说明**：提供资源控制功能

### 7. alert（告警模块）
- **依赖**：
  - `yw_utils` (PRIVATE) - 使用 JsonConfig, IPAddressUtils, TimeUtils, DurationUtils, PostgreSQLConnectionPool
  - `pqxx` (PostgreSQL客户端，通过 yw_utils 的 PUBLIC 链接自动传递)
  - `spdlog`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **被依赖**：
  - `web` (PUBLIC)
- **说明**：处理告警规则和事件，使用DDD分层架构。使用 `IPAddressUtils` 进行IP地址与机箱槽位转换，使用 `PostgreSQLConnectionPool` 进行数据库连接管理。不再依赖 `node::INodeModule`

### 8. web（Web模块）
- **依赖**：
  - `yw_alert` (PUBLIC) - 依赖会传递给链接 yw_web 的目标
  - `yw_utils` (PRIVATE) - 使用 ResponseBuilder 构建HTTP响应
  - `spdlog`, `hv_static`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **运行时依赖**（通过接口注入）：
  - `node::INodeModule`
  - `monitor::IMonitorModule`
  - `bmc::IBMCModule`
  - `controller::IControllerModule`
  - `alert::IAlertModule`
- **说明**：提供HTTP API路由，聚合所有业务模块的功能。使用 `yw::utils::ResponseBuilder` 构建标准化的HTTP响应

### 9. app（应用入口）
- **依赖**：
  - `yw_utils` (PRIVATE) - 使用 JsonConfig 读取配置
  - 所有业务模块：
    - `yw_core` (使用 AppContext)
    - `yw_node`
    - `yw_monitor`
    - `yw_web`
    - `yw_bmc`
    - `yw_controller`
  - `spdlog`, `hv_static`, `nlohmann_json` 通过 `yw_utils` 的 PUBLIC 链接自动传递
- **说明**：负责模块初始化、依赖注入和启动

## 运行时依赖关系

### 模块创建顺序（在 `app/main.cpp` 中）

```cpp
1. node_module      // 创建节点模块
2. monitor_module   // 创建监控模块
3. bmc_module       // 创建BMC模块
4. controller_module // 创建控制器模块
5. alert_module     // 创建告警模块（独立创建，不再依赖 node_module）
6. web_module       // 创建Web模块（依赖所有其他模块）
```

### 接口注入关系

```
app (main.cpp)
  ├─> node_module
  ├─> monitor_module
  ├─> bmc_module
  ├─> controller_module
  ├─> alert_module (独立创建，不再需要 node_module 注入)
  └─> web_module (注入所有模块)
```

## 依赖关系图

### 编译时依赖（CMake链接）

```
┌─────────┐
│   app   │──> yw_core ──> yw_utils
└────┬────┘   │
     │        │
     ├─> yw_node ──────> yw_utils ──> [spdlog, hv, nlohmann_json]
     ├─> yw_monitor ────> yw_utils ──> [spdlog, hv, nlohmann_json]
     ├─> yw_web ────────> yw_utils ──> [spdlog, hv, nlohmann_json]
     ├─> yw_bmc ────────> yw_utils ──> [spdlog, nlohmann_json]
     ├─> yw_alert ──────> yw_utils ──> [spdlog, nlohmann_json, pqxx]
     └─> yw_controller ─> yw_utils ──> [spdlog]
           │
           └─> yw_utils ──> [spdlog, hv, nlohmann_json]

说明：
- yw_utils 使用 PUBLIC 链接第三方库（spdlog, hv_static, nlohmann_json, pqxx），依赖会自动传递
- 依赖 yw_utils 的模块不需要显式链接第三方库，通过 yw_utils 的 PUBLIC 链接自动传递
- yw_utils 被多个模块依赖，提供 JsonConfig、MulticastScanner、DurationUtils、TimeUtils、IPAddressUtils、PostgreSQLConnectionPool、ResponseBuilder 等工具
- yw_web 依赖 yw_utils 以使用 ResponseBuilder 构建HTTP响应
- yw_alert、yw_monitor、yw_bmc 使用 PostgreSQLConnectionPool 进行数据库连接管理
```

### 运行时依赖（接口注入）

```
app
 │
 ├─> node_module
 │     └─> (注入到) web_module
 │
 ├─> monitor_module
 │     └─> (注入到) web_module
 │
 ├─> bmc_module
 │     └─> (注入到) web_module
 │
 ├─> controller_module
 │     └─> (注入到) web_module
 │
 ├─> alert_module (独立创建，使用 IPAddressUtils 替代 node_module)
 │     └─> (注入到) web_module
 │
 └─> web_module
       ├─> node_module (接口)
       ├─> monitor_module (接口)
       ├─> bmc_module (接口)
       ├─> controller_module (接口)
       └─> alert_module (接口)
```

## 关键设计模式

### 1. 依赖注入（Dependency Injection）
- 所有模块通过接口（抽象基类）进行交互
- 运行时通过工厂模式创建实例并注入依赖
- 避免编译时循环依赖

### 2. 接口隔离
- 每个模块提供清晰的接口定义（在 `include/` 目录）
- 实现细节隐藏在 `src/` 目录
- 模块之间通过接口通信，不直接依赖实现

### 3. 分层架构
- **alert模块**使用DDD分层架构：
  - Domain Layer（领域层）
  - Application Layer（应用层）
  - Infrastructure Layer（基础设施层）

## 依赖传递规则

### PUBLIC 链接
- `yw_alert` → `yw_web` (PUBLIC)
- 依赖会传递给链接 `yw_web` 的目标（如 `yw_app`）

### 第三方库链接策略
- `yw_utils` 使用 `PUBLIC` 链接第三方库（spdlog, hv_static, nlohmann_json），依赖会自动传递
- 依赖 `yw_utils` 的模块不需要显式链接这些第三方库，通过 `yw_utils` 的 `PUBLIC` 链接自动传递
- 各模块只需链接 `yw_utils`，即可自动获得所需的第三方库依赖
- 不再通过 `yw_core` 传递第三方库依赖
- `yw_core` 的第三方库依赖改为通过 `yw_utils` 传递

### PRIVATE 链接
- 大部分模块依赖都是 PRIVATE
- 依赖不会传递给链接该模块的目标
- 减少不必要的依赖传递

## 注意事项

1. **避免循环依赖**：
   - 所有模块依赖都是单向的
   - 通过接口注入避免编译时循环依赖

2. **接口优先**：
   - 模块之间通过接口（抽象基类）通信
   - 接口定义在 `include/` 目录
   - 实现细节在 `src/` 目录

3. **依赖最小化**：
   - 尽量使用 PRIVATE 链接
   - 只在必要时使用 PUBLIC 链接
   - 通过接口注入减少编译时依赖

4. **模块独立性**：
   - 每个模块可以独立编译
   - 模块之间通过接口解耦
   - 便于测试和维护

5. **工具类组织**：
   - 通用工具类应放在 `yw_utils` 模块中
   - `ResponseBuilder` 已从 `src/web/utils/` 移动到 `yw_utils` 模块
   - `PostgreSQLConnectionPool` 已从 `src/alert/infrastructure/` 移动到 `yw_utils` 模块，供所有模块复用
   - 工具类的实现应拆分到 `.cpp` 文件，模板函数保留在头文件

6. **数据库连接管理**：
   - 所有使用 PostgreSQL 的模块（alert、monitor、bmc）都使用 `PostgreSQLConnectionPool` 进行连接管理
   - 连接池提供连接复用、线程安全、自动重连等功能
   - 通过 `ConnectionGuard` (RAII) 自动管理连接的获取和释放

7. **IP地址解析**：
   - `alert` 模块使用 `IPAddressUtils` 工具类进行IP地址与机箱槽位转换
   - 不再依赖 `node::INodeModule` 获取节点信息
   - 提高了模块独立性和性能

