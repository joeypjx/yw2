# 项目依赖分析文档

本文档详细说明本工程的所有依赖库，包括 `third_party` 目录下的库和系统/外部库。

## 依赖分类

### 一、third_party 目录下的第三方库

这些库位于 `third_party/` 目录，通过 `add_subdirectory()` 集成到 CMake 构建系统中。

#### 1. **spdlog** - 日志库
- **位置**: `third_party/spdlog`
- **用途**: 日志记录
- **链接方式**: 通过 `yw_utils` 的 PUBLIC 链接传递
- **使用模块**: 所有模块（通过 yw_utils）

#### 2. **libhv** - 网络库
- **位置**: `third_party/libhv`
- **用途**: HTTP 服务器、网络通信
- **链接方式**: 通过 `yw_utils` 的 PUBLIC 链接传递
- **使用模块**: web 模块（通过 yw_utils）

#### 3. **nlohmann/json** - JSON 库
- **位置**: `third_party/json`
- **用途**: JSON 序列化/反序列化
- **链接方式**: 通过 `yw_utils` 的 PUBLIC 链接传递
- **使用模块**: 所有模块（通过 yw_utils）

#### 4. **eventpp** - 事件库
- **位置**: `third_party/eventpp`
- **用途**: 事件分发和处理
- **链接方式**: 直接链接（如需要）
- **使用模块**: 可能用于事件驱动架构

#### 5. **libpqxx** - PostgreSQL C++ 客户端库
- **位置**: `third_party/libpqxx`
- **用途**: PostgreSQL 数据库访问
- **链接方式**: 直接链接到 `yw_utils`, `yw_alert`, `yw_bmc`, `yw_monitor`
- **使用模块**: 
  - `yw_utils` (PUBLIC)
  - `yw_alert` (PRIVATE)
  - `yw_bmc` (PRIVATE)
  - `yw_monitor` (PRIVATE)
- **依赖**: 需要 libpq（PostgreSQL 客户端库）

#### 6. **libpq** - PostgreSQL 客户端库
- **位置**: `third_party/postgres/src/interfaces/libpq`
- **用途**: PostgreSQL 底层客户端库
- **链接方式**: 静态链接（libpq.a, libpgcommon.a, libpgport.a）
- **使用模块**: 通过 libpqxx 间接使用
- **配置**: 在 CMakeLists.txt 中手动配置路径

#### 7. **libfreeipmi** - FreeIPMI 库
- **位置**: `third_party/libfreeipmi`
- **用途**: IPMI 协议实现（开源版本）
- **链接方式**: 通过 `FREEIPMI_LIBRARIES` 变量链接
- **使用模块**: `yw_ipmi`（当前已注释）
- **配置**: 在 CMakeLists.txt 中手动配置路径

### 二、系统库（不在 third_party 目录）

这些是系统提供的标准库或通过包管理器安装的库。

#### 1. **Threads (pthread)** - 线程库
- **类型**: 系统库
- **用途**: 多线程支持
- **链接方式**: 通过 `find_package(Threads REQUIRED)` 和 `Threads::Threads`
- **使用模块**: `yw_ipmi`
- **说明**: POSIX 线程库，Linux/macOS 系统自带

#### 2. **dl** - 动态链接库
- **类型**: 系统库
- **用途**: 动态库加载（dlopen, dlsym 等）
- **链接方式**: 直接链接 `dl`
- **使用模块**: `yw_ipmi`
- **说明**: Linux 系统库，用于运行时加载共享库

### 三、外部第三方库（需要系统安装）

这些库不在 `third_party` 目录，需要通过系统包管理器安装。

#### 1. **libgcrypt** - 加密库
- **类型**: 外部库
- **用途**: 加密算法实现
- **链接方式**: 直接链接 `gcrypt`
- **使用模块**: `yw_ipmi`
- **安装方式**: 
  - macOS: `brew install libgcrypt`
  - Linux: `apt-get install libgcrypt-dev` 或 `yum install libgcrypt-devel`
- **说明**: GNU 加密库，提供各种加密算法

#### 2. **libgpg-error** - GPG 错误处理库
- **类型**: 外部库
- **用途**: GPG 相关错误处理
- **链接方式**: 直接链接 `gpg-error`
- **使用模块**: `yw_ipmi`
- **安装方式**: 
  - macOS: `brew install libgpg-error`
  - Linux: `apt-get install libgpg-error-dev` 或 `yum install libgpg-error-devel`
- **说明**: libgcrypt 的依赖库

## 依赖关系图

```
yw_app (可执行文件)
├── yw_core
├── yw_node
├── yw_monitor
│   ├── yw_utils (PUBLIC)
│   │   ├── spdlog (PUBLIC)
│   │   ├── nlohmann_json (PUBLIC)
│   │   ├── hv_static (PUBLIC)
│   │   └── pqxx (PUBLIC)
│   └── pqxx (PRIVATE)
├── yw_web
│   ├── yw_alert (PUBLIC)
│   │   ├── yw_utils (PRIVATE)
│   │   └── pqxx (PRIVATE)
│   └── yw_utils (PRIVATE)
├── yw_bmc
│   ├── yw_utils (PRIVATE)
│   └── pqxx (PRIVATE)
├── yw_controller
└── yw_utils (PRIVATE)

yw_ipmi (已注释，未使用)
├── libfreeipmi (third_party)
├── gcrypt (系统安装)
├── gpg-error (系统安装)
├── Threads::Threads (系统库)
└── dl (系统库)
```

## 依赖传递说明

### PUBLIC 链接
- **yw_utils** 使用 PUBLIC 链接 `spdlog`, `nlohmann_json`, `hv_static`, `pqxx`
- 这意味着任何链接 `yw_utils` 的模块都会自动获得这些依赖
- 例如：`yw_alert` 链接 `yw_utils` 后，自动可以使用 `spdlog` 和 `nlohmann_json`

### PRIVATE 链接
- 大多数模块使用 PRIVATE 链接，依赖不会传递给链接该模块的其他模块
- 例如：`yw_alert` PRIVATE 链接 `pqxx`，链接 `yw_alert` 的模块不会自动获得 `pqxx`

## 系统依赖安装

### macOS (Homebrew)
```bash
# IPMI 相关库（如果使用 yw_ipmi 模块）
brew install libgcrypt libgpg-error

# PostgreSQL 开发库（如果使用系统安装的 PostgreSQL）
brew install postgresql
```

### Linux (Ubuntu/Debian)
```bash
# IPMI 相关库
sudo apt-get install libgcrypt20-dev libgpg-error-dev

# PostgreSQL 开发库
sudo apt-get install libpq-dev postgresql-server-dev-all
```

### Linux (CentOS/RHEL)
```bash
# IPMI 相关库
sudo yum install libgcrypt-devel libgpg-error-devel

# PostgreSQL 开发库
sudo yum install postgresql-devel postgresql-server-devel
```

## 编译时依赖检查

### 必需依赖（当前使用）
1. ✅ **spdlog** - third_party
2. ✅ **libhv** - third_party
3. ✅ **nlohmann/json** - third_party
4. ✅ **libpqxx** - third_party
5. ✅ **libpq** - third_party/postgres
6. ✅ **Threads** - 系统库（如果使用 yw_ipmi）

### 可选依赖（当前未使用）
1. ⚠️ **libfreeipmi** - third_party（yw_ipmi 模块已注释）
2. ⚠️ **gcrypt** - 系统安装（yw_ipmi 模块已注释）
3. ⚠️ **gpg-error** - 系统安装（yw_ipmi 模块已注释）
4. ⚠️ **dl** - 系统库（yw_ipmi 模块已注释）

## 注意事项

1. **libpq 和 libpqxx**：
   - `libpqxx` 依赖 `libpq`
   - 项目使用 `third_party/postgres` 中的静态库版本
   - 需要确保 `libpq.a`, `libpgcommon.a`, `libpgport.a` 已构建

2. **IPMI 模块**：
   - `yw_ipmi` 模块当前已注释，不参与编译
   - 如果启用，需要安装 `libgcrypt` 和 `libgpg-error`
   - 需要确保 `libfreeipmi` 已构建

3. **依赖传递**：
   - 通过 `yw_utils` 的 PUBLIC 链接，大多数模块自动获得常用库
   - 减少重复链接，简化依赖管理

4. **系统库**：
   - `Threads` 和 `dl` 是系统标准库，通常不需要额外安装
   - 在 macOS 和 Linux 上通常默认可用

## 总结

**除了 `third_party` 目录下的库，本工程还依赖：**

1. **系统库**（2个）：
   - `Threads` (pthread) - 多线程支持
   - `dl` - 动态库加载

2. **外部第三方库**（2个，当前未使用）：
   - `libgcrypt` - 加密库（yw_ipmi 模块需要）
   - `libgpg-error` - GPG 错误处理库（yw_ipmi 模块需要）

**注意**：`yw_ipmi` 模块当前已注释，因此 `libgcrypt` 和 `libgpg-error` 实际上不需要安装。如果将来启用 IPMI 模块，则需要安装这两个库。

