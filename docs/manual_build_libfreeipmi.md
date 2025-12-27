# 手动构建 libfreeipmi

## 前提条件

确保已安装以下依赖：
- `libgcrypt` 和 `libgpg-error`（通过 Homebrew 安装）
- `argp-standalone`（macOS 需要，通过 Homebrew 安装）
- `texinfo`（可选，如果不禁用文档生成则需要）
- `gcc` 或 `clang` 编译器
- `make` 工具
- `autotools`（如果需要从源码生成 configure 脚本）

### 安装依赖

```bash
brew install libgcrypt libgpg-error argp-standalone
# texinfo 是可选的，如果使用 --disable-doc 选项则不需要
# brew install texinfo
```

## 构建步骤

### 1. 进入 libfreeipmi 目录

```bash
cd /Users/panjinxue/编程/yw2/third_party/libfreeipmi
```

### 2. 清理之前的配置（可选）

如果之前已经运行过 `configure`，想要重新配置：

```bash
make distclean  # 清理构建文件但保留 configure 脚本
# 或者
rm -f config.status config.log Makefile
```

### 3. 生成 configure 脚本（如果需要）

如果 `configure` 脚本不存在，需要先运行：

```bash
./autogen.sh
```

### 4. 配置构建选项

有两种方式：

#### 方式 A：使用 libgcrypt 加密支持（推荐）

设置环境变量并运行 configure：

```bash
# 使用 pkg-config 自动获取路径（推荐）
export CPPFLAGS="$(pkg-config --cflags libgcrypt libgpg-error) -I/opt/homebrew/include"
export CFLAGS="$CPPFLAGS"
export LDFLAGS="-L/opt/homebrew/lib"
export LIBS="-largp"
./configure --prefix=/Users/panjinxue/编程/yw2/third_party/libfreeipmi/install
```

或者手动指定路径（如果 pkg-config 不可用）：

```bash
# Homebrew 通常将头文件链接到 /opt/homebrew/include
export CPPFLAGS="-I/opt/homebrew/Cellar/libgcrypt/1.11.2/include -I/opt/homebrew/opt/libgpg-error/include -I/opt/homebrew/include"
export CFLAGS="-I/opt/homebrew/Cellar/libgcrypt/1.11.2/include -I/opt/homebrew/opt/libgpg-error/include -I/opt/homebrew/include"
export LDFLAGS="-L/opt/homebrew/lib"
export LIBS="-largp"
./configure --prefix=/Users/panjinxue/编程/yw2/third_party/libfreeipmi/install --disable-doc
```

#### 方式 B：不使用加密支持（如果找不到 libgcrypt）

```bash
export CPPFLAGS="-I/opt/homebrew/include"
export CFLAGS="-I/opt/homebrew/include"
export LDFLAGS="-L/opt/homebrew/lib"
export LIBS="-largp"
./configure --prefix=/Users/panjinxue/编程/yw2/third_party/libfreeipmi/install --without-encryption --disable-doc
```

### 5. 编译

```bash
make -j$(sysctl -n hw.ncpu)
```

或者使用固定数量的并行任务：

```bash
make -j4
```

### 6. 安装（可选）

如果需要安装到系统目录：

```bash
make install
```

**注意**：对于本项目，我们只需要编译好的静态库文件，不需要安装。编译完成后，静态库文件位于：

```
libfreeipmi/.libs/libfreeipmi.a
```

## 验证构建结果

检查静态库是否生成：

```bash
ls -lh libfreeipmi/.libs/libfreeipmi.a
```

如果文件存在且大小合理（通常几 MB），说明构建成功。

## 常见问题

### 问题 1：`gcrypt.h not found`

**原因**：configure 脚本找不到 libgcrypt 的头文件。

**解决方法**：
1. 确保已安装 libgcrypt：`brew install libgcrypt`
2. 使用 `pkg-config` 查找正确的路径：
   ```bash
   pkg-config --cflags libgcrypt
   pkg-config --cflags libgpg-error
   ```
3. 将输出结果设置到 `CPPFLAGS` 和 `CFLAGS` 环境变量中
4. 或者使用 `--without-encryption` 选项跳过加密支持

### 问题 2：`makeinfo: command not found`

**原因**：构建过程中需要生成文档，但系统缺少 `makeinfo` 工具（来自 `texinfo` 包）。

**解决方法**（两种方式）：

**方式 A：禁用文档生成（推荐，更快）**
```bash
./configure --prefix=... --disable-doc
```

**方式 B：安装 texinfo**
```bash
brew install texinfo
```

### 问题 3：`argp library not found. argp-standalone required.`

**原因**：macOS 系统没有内置 argp 库，需要安装 argp-standalone。

**解决方法**：
```bash
brew install argp-standalone
```

然后设置环境变量：
```bash
export CPPFLAGS="-I/opt/homebrew/include"
export LDFLAGS="-L/opt/homebrew/lib"
export LIBS="-largp"
```

### 问题 2：configure 脚本不存在

**解决方法**：
```bash
./autogen.sh
```

如果 `autogen.sh` 也不存在，可能需要从源码仓库重新获取，或者使用已发布的 tarball。

### 问题 3：编译错误

**解决方法**：
1. 检查编译器是否正确安装：`gcc --version`
2. 查看详细的错误信息：`make VERBOSE=1`
3. 尝试清理后重新构建：`make clean && make`

## 完整示例脚本

```bash
#!/bin/bash

# 设置工作目录
cd /Users/panjinxue/编程/yw2/third_party/libfreeipmi

# 清理之前的构建
make distclean 2>/dev/null || true
rm -f config.status config.log

# 设置环境变量（使用 pkg-config 获取路径）
export CPPFLAGS="$(pkg-config --cflags libgcrypt libgpg-error 2>/dev/null || echo '-I/opt/homebrew/include')"
export CFLAGS="$CPPFLAGS"
export LDFLAGS="-L/opt/homebrew/lib"
export LIBS="-largp"

# 配置（使用 --disable-doc 跳过文档生成，加快构建速度）
./configure --prefix=/Users/panjinxue/编程/yw2/third_party/libfreeipmi/install --disable-doc

# 编译
make -j$(sysctl -n hw.ncpu)

# 验证
if [ -f "libfreeipmi/.libs/libfreeipmi.a" ]; then
    echo "构建成功！静态库位于: libfreeipmi/.libs/libfreeipmi.a"
    ls -lh libfreeipmi/.libs/libfreeipmi.a
else
    echo "构建失败！"
    exit 1
fi
```

## 与 CMake 集成

手动构建完成后，CMake 会自动检测到已存在的静态库文件，跳过自动构建步骤。如果需要强制 CMake 重新构建，可以：

1. 删除静态库文件：
   ```bash
   rm -f libfreeipmi/.libs/libfreeipmi.a
   ```

2. 或者在 CMake 配置时启用强制重建选项：
   ```bash
   cmake .. -DFORCE_REBUILD_LIBFREEIPMI=ON
   ```

