#!/usr/bin/env bash

set -euo pipefail
shopt -s nullglob

# === 基本路径与默认参数 ===
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
SQL_DIR="${SCRIPT_DIR}"

# Docker insecure registries
INSECURE_REGISTRIES_DEFAULT=("127.0.0.1:5000" "localhost:5000")
DAEMON_JSON=${DAEMON_JSON:-/etc/docker/daemon.json}

# MySQL 配置
MYSQL_IMAGE=${MYSQL_IMAGE:-mysql:8.0}
MYSQL_NAME=${MYSQL_NAME:-yw-mysql}
MYSQL_ROOT_PASSWORD=${MYSQL_ROOT_PASSWORD:-yw_root_pwd}
MYSQL_DATABASE=${MYSQL_DATABASE:-yw}
MYSQL_USER=${MYSQL_USER:-yw}
MYSQL_PASSWORD=${MYSQL_PASSWORD:-yw_pwd}
MYSQL_DATA_DIR=${MYSQL_DATA_DIR:-/opt/yw/mysql/data}
MYSQL_CONF_DIR=${MYSQL_CONF_DIR:-/opt/yw/mysql/conf}

# TimescaleDB/PostgreSQL 配置
TS_IMAGE=${TS_IMAGE:-timescale/timescaledb:latest-pg14}
TS_NAME=${TS_NAME:-yw-timescaledb}
POSTGRES_USER=${POSTGRES_USER:-postgres}
POSTGRES_PASSWORD=${POSTGRES_PASSWORD:-postgres}
POSTGRES_DB=${POSTGRES_DB:-ywts}
TS_DATA_DIR=${TS_DATA_DIR:-/opt/yw/timescaledb/data}
TS_CONF_DIR=${TS_CONF_DIR:-/opt/yw/timescaledb/conf}

# Docker Registry 配置
REGISTRY_IMAGE=${REGISTRY_IMAGE:-registry:2}
REGISTRY_NAME=${REGISTRY_NAME:-yw-registry}
REGISTRY_DATA_DIR=${REGISTRY_DATA_DIR:-/opt/yw/registry/data}
REGISTRY_CONFIG_DIR=${REGISTRY_CONFIG_DIR:-/opt/yw/registry/conf}
REGISTRY_LISTEN_ADDR=${REGISTRY_LISTEN_ADDR:-:5000}

# Nginx 配置
NGINX_IMAGE=${NGINX_IMAGE:-nginx:1.25-alpine}
NGINX_NAME=${NGINX_NAME:-yw-nginx}
NGINX_CONF_DIR=${NGINX_CONF_DIR:-/opt/yw/nginx/conf}
NGINX_HTML_DIR=${NGINX_HTML_DIR:-/opt/yw/nginx/html}

# Service（将同目录可执行文件注册为 systemd 服务）
SERVICE_NAME=${SERVICE_NAME:-yw-app}
SERVICE_EXEC=${SERVICE_EXEC:-yw_app}
SERVICE_USER=${SERVICE_USER:-root}
SERVICE_ENV_FILE=${SERVICE_ENV_FILE:-${SCRIPT_DIR}/service.env}
SERVICE_UNIT_PATH=${SERVICE_UNIT_PATH:-/etc/systemd/system/${SERVICE_NAME}.service}

# === 工具函数 ===
log() { echo "[install] $*"; }
warn() { echo "[install][WARN] $*" >&2; }
err() { echo "[install][ERROR] $*" >&2; }

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    err "未找到命令: $1，请先安装后再运行本脚本。"; exit 1
  fi
}

ensure_dir() {
  local d="$1"
  if [ ! -d "$d" ]; then
    mkdir -p "$d"
  fi
}

ensure_linux() {
  local os
  os=$(uname -s || true)
  if [ "${os}" != "Linux" ]; then
    err "部署环境需为 Linux（当前: ${os}）。host 网络模式在非 Linux 上不可用。"; exit 1
  fi
}

ensure_root() {
  if [ "${EUID:-$(id -u)}" -ne 0 ]; then
    err "请以 root 身份运行（或使用 sudo）。"; exit 1
  fi
}

container_exists() {
  local name="$1"
  docker ps -a --format '{{.Names}}' | grep -x "$name" >/dev/null 2>&1
}

container_running() {
  local name="$1"
  docker ps --format '{{.Names}}' | grep -x "$name" >/dev/null 2>&1
}

start_container_if_stopped() {
  local name="$1"
  if container_exists "$name" && ! container_running "$name"; then
    log "启动已存在但停止的容器: ${name}"
    docker start "$name" >/dev/null
  fi
}


# === Docker daemon: 增加不安全仓库并重启 ===
add_insecure_registries_and_restart() {
  local changed=0

  if [ ! -f "$DAEMON_JSON" ]; then
    log "创建 ${DAEMON_JSON} 并写入 insecure-registries"
    ensure_dir "$(dirname "$DAEMON_JSON")"
    {
      echo '{'
      echo '  "insecure-registries": ['
      local i
      for i in "${!INSECURE_REGISTRIES_DEFAULT[@]}"; do
        local reg="${INSECURE_REGISTRIES_DEFAULT[$i]}"
        if [ "$i" -gt 0 ]; then echo ","; fi
        printf '    "%s"' "$reg"
      done
      echo ''
      echo '  ]'
      echo '}'
    } >"$DAEMON_JSON"
    changed=1
  else
    local reg
    for reg in "${INSECURE_REGISTRIES_DEFAULT[@]}"; do
      if command -v jq >/dev/null 2>&1; then
        # 使用 jq 合并/去重
        if ! jq -e --arg r "$reg" '."insecure-registries" // [] | index($r)' "$DAEMON_JSON" >/dev/null; then
          log "写入不安全仓库: $reg"
          tmpfile=$(mktemp)
          jq --arg r "$reg" '."insecure-registries" = ((."insecure-registries" // []) + [$r] | unique)' "$DAEMON_JSON" >"$tmpfile"
          mv "$tmpfile" "$DAEMON_JSON"
          changed=1
        fi
      else
        # 使用 python3 进行安全 JSON 修改
        if command -v python3 >/dev/null 2>&1; then
          if python3 - "$DAEMON_JSON" "$reg" <<'PY'
import json, sys, os, shutil
path, reg = sys.argv[1], sys.argv[2]
try:
    data = {}
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    arr = list(dict.fromkeys(list(data.get('insecure-registries', [])) + [reg]))
    if reg not in data.get('insecure-registries', []):
        data['insecure-registries'] = arr
        tmp = path + '.tmp'
        with open(tmp, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
            f.write('\n')
        shutil.move(tmp, path)
        sys.stdout.write('CHANGED')
except Exception as e:
    sys.stderr.write(f'python3 修改 {path} 失败: {e}\n')
    sys.exit(1)
PY
          then
            changed=1
          fi
        else
          err "缺少 jq 与 python3，无法自动更新 ${DAEMON_JSON}。"
          err "请手动加入 insecure-registries: ${INSECURE_REGISTRIES_DEFAULT[*]}"
        fi
      fi
    done
  fi

  if [ "$changed" -eq 1 ]; then
    log "重启 Docker 服务以应用 daemon.json 变更"
    if command -v systemctl >/dev/null 2>&1; then
      systemctl restart docker
    elif command -v service >/dev/null 2>&1; then
      service docker restart
    else
      warn "找不到 systemctl/service，无法自动重启 Docker，请手动重启。"
    fi
  else
    log "daemon.json 无需变更"
  fi
}


# === SQL 导入（从脚本所在目录） ===
MYSQL_SQL_GLOBS=("mysql_*.sql" "*.mysql.sql")
PG_SQL_GLOBS=("timescaledb_*.sql" "bmc_timescaledb_*.sql" "pg_*.sql" "*.pgsql.sql" "*.psql.sql")

import_mysql_sql_from_dir() {
  local dir="$1"
  local any=0
  local pattern file
  for pattern in "${MYSQL_SQL_GLOBS[@]}"; do
    for file in "${dir}"/${pattern}; do
      if [ -f "$file" ]; then
        any=1
        log "导入 MySQL SQL: $(basename "$file")"
        docker exec -i "$MYSQL_NAME" mysql -uroot -p"${MYSQL_ROOT_PASSWORD}" "$MYSQL_DATABASE" <"$file"
      fi
    done
  done
  if [ "$any" -eq 1 ]; then return 0; else return 1; fi
}

import_pg_sql_from_dir() {
  local dir="$1"
  local any=0
  local pattern file
  for pattern in "${PG_SQL_GLOBS[@]}"; do
    for file in "${dir}"/${pattern}; do
      if [ -f "$file" ]; then
        any=1
        log "导入 TimescaleDB SQL: $(basename "$file")"
        docker exec -i "$TS_NAME" bash -lc "psql -v ON_ERROR_STOP=1 -U '${POSTGRES_USER}' -d '${POSTGRES_DB}' -f -" <"$file"
      fi
    done
  done
  if [ "$any" -eq 1 ]; then return 0; else return 1; fi
}


# === 加载脚本目录下的 Docker 镜像（*.tar） ===
load_docker_images_from_dir() {
  local dir="$1"
  local any=0
  local file
  for file in "${dir}"/*.tar; do
    if [ -f "$file" ]; then
      any=1
      log "加载 Docker 镜像: $(basename "$file")"
      if ! docker load -i "$file"; then
        err "加载失败: $file"; exit 1
      fi
    fi
  done
  if [ "$any" -eq 0 ]; then
    warn "未在 ${dir} 发现 .tar 镜像文件"
  fi
}


# === 部署 MySQL 容器与初始化 ===
wait_mysql_ready() {
  local name="$1"
  local retries=60
  while (( retries > 0 )); do
    if docker exec "$name" mysqladmin ping -uroot -p"${MYSQL_ROOT_PASSWORD}" --silent >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
    retries=$((retries-1))
  done
  return 1
}

deploy_mysql() {
  ensure_dir "$MYSQL_DATA_DIR"
  ensure_dir "$MYSQL_CONF_DIR"

  if container_exists "$MYSQL_NAME"; then
    log "MySQL 容器已存在: ${MYSQL_NAME}"
    start_container_if_stopped "$MYSQL_NAME"
  else
    log "创建并启动 MySQL 容器: ${MYSQL_NAME}"
    docker run -d --name "$MYSQL_NAME" \
      --network host \
      -e MYSQL_ROOT_PASSWORD="$MYSQL_ROOT_PASSWORD" \
      -e MYSQL_DATABASE="$MYSQL_DATABASE" \
      -e MYSQL_USER="$MYSQL_USER" \
      -e MYSQL_PASSWORD="$MYSQL_PASSWORD" \
      -v "$MYSQL_DATA_DIR":/var/lib/mysql \
      -v "$MYSQL_CONF_DIR":/etc/mysql/conf.d \
      "$MYSQL_IMAGE" --default-authentication-plugin=mysql_native_password >/dev/null
  fi

  log "等待 MySQL 就绪..."
  if ! wait_mysql_ready "$MYSQL_NAME"; then
    err "等待 MySQL 就绪超时"; exit 1
  fi

#   docker exec -i "$MYSQL_NAME" mysql -uroot -p"${MYSQL_ROOT_PASSWORD}" "$MYSQL_DATABASE" <<'SQL'
# GRANT ALL PRIVILEGES ON *.* TO '${MYSQL_USER}'@'%' WITH GRANT OPTION; FLUSH PRIVILEGES;
# SQL

  # 优先导入脚本目录中的 MySQL SQL 文件；若未提供则执行默认初始化
  if ! import_mysql_sql_from_dir "$SQL_DIR"; then
    log "未发现 MySQL SQL 文件，执行默认初始化表"
    docker exec -i "$MYSQL_NAME" mysql -uroot -p"${MYSQL_ROOT_PASSWORD}" "$MYSQL_DATABASE" <<'SQL'
CREATE TABLE IF NOT EXISTS test (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
SQL
  fi
}


# === 部署 TimescaleDB 容器与加载 SQL ===
wait_ts_ready() {
  local name="$1"
  local retries=60
  while (( retries > 0 )); do
    if docker exec "$name" pg_isready -U "$POSTGRES_USER" >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
    retries=$((retries-1))
  done
  return 1
}

deploy_timescaledb() {
  ensure_dir "$TS_DATA_DIR"
  ensure_dir "$TS_CONF_DIR"

  if container_exists "$TS_NAME"; then
    log "TimescaleDB 容器已存在: ${TS_NAME}"
    start_container_if_stopped "$TS_NAME"
  else
    log "创建并启动 TimescaleDB 容器: ${TS_NAME}"
    docker run -d --name "$TS_NAME" \
      --network host \
      -e POSTGRES_USER="$POSTGRES_USER" \
      -e POSTGRES_PASSWORD="$POSTGRES_PASSWORD" \
      -e POSTGRES_DB="$POSTGRES_DB" \
      -v "$TS_DATA_DIR":/var/lib/postgresql/data \
      -v "$TS_CONF_DIR":/etc/postgresql \
      "$TS_IMAGE" >/dev/null
  fi

  log "等待 TimescaleDB 就绪..."
  if ! wait_ts_ready "$TS_NAME"; then
    err "等待 TimescaleDB 就绪超时"; exit 1
  fi

  # 从脚本目录导入 TimescaleDB/PostgreSQL SQL（若存在）
  if ! import_pg_sql_from_dir "$SQL_DIR"; then
    log "未发现 TimescaleDB/PostgreSQL SQL 文件，跳过导入"
  fi
}


# === 部署本地 Docker Registry ===
deploy_registry() {
  ensure_dir "$REGISTRY_DATA_DIR"
  ensure_dir "$REGISTRY_CONFIG_DIR"

  if container_exists "$REGISTRY_NAME"; then
    log "Registry 容器已存在: ${REGISTRY_NAME}"
    start_container_if_stopped "$REGISTRY_NAME"
  else
    log "创建并启动 Registry 容器: ${REGISTRY_NAME}"
    docker run -d --name "$REGISTRY_NAME" \
      --network host \
      -e REGISTRY_HTTP_ADDR="$REGISTRY_LISTEN_ADDR" \
      -v "$REGISTRY_DATA_DIR":/var/lib/registry \
      -v "$REGISTRY_CONFIG_DIR":/etc/docker/registry \
      "$REGISTRY_IMAGE" >/dev/null
  fi
}


# === 部署 Nginx ===
ensure_default_nginx_conf() {
  ensure_dir "$NGINX_CONF_DIR"
  ensure_dir "$NGINX_HTML_DIR"

  local conf_file="$NGINX_CONF_DIR/nginx.conf"
  if [ ! -f "$conf_file" ]; then
    log "生成默认 Nginx 配置: $conf_file"
    cat >"$conf_file" <<'NGINX'
user  nginx;
worker_processes  auto;

error_log  /var/log/nginx/error.log warn;
pid        /var/run/nginx.pid;

events {
    worker_connections  1024;
}

http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';

    access_log  /var/log/nginx/access.log  main;

    sendfile        on;
    keepalive_timeout  65;

    server {
        listen       80;
        server_name  localhost;

        location / {
            root   /usr/share/nginx/html;
            index  index.html index.htm;
        }
    }
}
NGINX
  fi

  local index_file="$NGINX_HTML_DIR/index.html"
  if [ ! -f "$index_file" ]; then
    log "生成默认 Nginx 首页: $index_file"
    cat >"$index_file" <<'HTML'
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>YW Nginx</title></head>
<body>
  <h1>YW Nginx 已运行</h1>
  <p>默认首页，可自行替换。</p>
  <p>时间: $(date)</p>
  <style> body{font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Fira Sans", "Droid Sans", "Helvetica Neue", Arial, "Noto Sans", "PingFang SC", "Hiragino Sans GB", "Microsoft YaHei", sans-serif;} </style>
  <hr/>
  <small>Generated by install.sh</small>
</body>
</html>
HTML
  fi
}

deploy_nginx() {
  ensure_default_nginx_conf

  if container_exists "$NGINX_NAME"; then
    log "Nginx 容器已存在: ${NGINX_NAME}"
    start_container_if_stopped "$NGINX_NAME"
  else
    log "创建并启动 Nginx 容器: ${NGINX_NAME}"
    docker run -d --name "$NGINX_NAME" \
      --network host \
      -v "$NGINX_CONF_DIR":/etc/nginx \
      -v "$NGINX_HTML_DIR":/usr/share/nginx/html \
      "$NGINX_IMAGE" >/dev/null
  fi
}


# === 安装为 systemd 服务（同目录可执行文件） ===
require_systemd() {
  if ! command -v systemctl >/dev/null 2>&1; then
    err "未检测到 systemd（systemctl 不存在），无法安装 Linux 服务。"; exit 1
  fi
}

render_service_unit() {
  local unit_path="$1"
  local exec_path="${SCRIPT_DIR}/${SERVICE_EXEC}"
  local work_dir="${SCRIPT_DIR}"
  cat >"$unit_path" <<UNIT
[Unit]
Description=YW App Service (${SERVICE_NAME})
After=network.target

[Service]
Type=simple
User=${SERVICE_USER}
WorkingDirectory=${work_dir}
EnvironmentFile=-/etc/default/${SERVICE_NAME}
EnvironmentFile=-${SERVICE_ENV_FILE}
ExecStart=${exec_path}
Restart=on-failure
RestartSec=3

[Install]
WantedBy=multi-user.target
UNIT
}

install_service() {
  require_systemd
  local exec_path="${SCRIPT_DIR}/${SERVICE_EXEC}"
  if [ ! -f "$exec_path" ]; then
    err "未找到可执行文件: $exec_path"
    exit 1
  fi
  if [ ! -x "$exec_path" ]; then
    log "为可执行文件增加可执行权限: $exec_path"
    chmod +x "$exec_path"
  fi

  log "写入 systemd unit: ${SERVICE_UNIT_PATH}"
  render_service_unit "${SERVICE_UNIT_PATH}"

  log "重新加载 systemd、启用并启动服务: ${SERVICE_NAME}"
  systemctl daemon-reload
  systemctl enable "${SERVICE_NAME}" >/dev/null
  systemctl restart "${SERVICE_NAME}"
  systemctl status "${SERVICE_NAME}" --no-pager | cat || true
}

uninstall_service() {
  require_systemd
  if systemctl list-units --full -all | grep -Fq "${SERVICE_NAME}.service"; then
    log "停止并禁用服务: ${SERVICE_NAME}"
    systemctl disable --now "${SERVICE_NAME}" || true
  fi
  if [ -f "${SERVICE_UNIT_PATH}" ]; then
    log "删除 unit 文件: ${SERVICE_UNIT_PATH}"
    rm -f "${SERVICE_UNIT_PATH}"
    systemctl daemon-reload
  fi
}

restart_service() {
  require_systemd
  systemctl restart "${SERVICE_NAME}"
  systemctl status "${SERVICE_NAME}" --no-pager | cat || true
}

status_service() {
  require_systemd
  systemctl status "${SERVICE_NAME}" --no-pager | cat || true
}


# === 主流程与子命令 ===
usage() {
  cat <<'USAGE'
用法: install.sh [步骤 ...]

步骤可选（可传多个，按顺序执行）:
  all               执行全部步骤
  daemon            配置 Docker daemon.json 并重启
  mysql             部署 MySQL 并导入脚本目录 SQL
  timescaledb       部署 TimescaleDB 并导入脚本目录 SQL
  registry          部署 Docker Registry
  nginx             部署 Nginx
  images-load       加载脚本目录下所有 *.tar Docker 镜像
  service-install   安装为 systemd 服务（同目录可执行文件）
  service-uninstall 卸载 systemd 服务
  service-restart   重启 systemd 服务
  service-status    查看 systemd 服务状态
  help              显示帮助

示例:
  sudo bash install.sh all
  sudo bash install.sh daemon
  sudo bash install.sh mysql timescaledb
  sudo bash install.sh images-load
  sudo bash install.sh service-install

可选环境变量:
  SERVICE_NAME       服务名（默认: yw-app）
  SERVICE_EXEC       同目录可执行文件名（默认: yw_app）
  SERVICE_USER       运行用户（默认: root）
  SERVICE_ENV_FILE   环境变量文件路径（默认: scripts 目录下 service.env）
USAGE
}

run_step() {
  local step="$1"
  case "$step" in
    daemon)
      log "配置 Docker insecure registries 并重启 Docker"
      add_insecure_registries_and_restart
      ;;
    mysql)
      log "部署 MySQL"
      deploy_mysql
      ;;
    timescaledb)
      log "部署 TimescaleDB"
      deploy_timescaledb
      ;;
    registry)
      log "部署 Docker Registry"
      deploy_registry
      ;;
    nginx)
      log "部署 Nginx"
      deploy_nginx
      ;;
    images-load)
      log "加载脚本目录下 .tar 镜像"
      load_docker_images_from_dir "$SQL_DIR"
      ;;
    service-install)
      log "安装 systemd 服务"
      install_service
      ;;
    service-uninstall)
      log "卸载 systemd 服务"
      uninstall_service
      ;;
    service-restart)
      log "重启 systemd 服务"
      restart_service
      ;;
    service-status)
      log "查看 systemd 服务状态"
      status_service
      ;;
    *)
      err "未知步骤: $step"; usage; exit 1;;
  esac
}

main() {
  ensure_linux
  ensure_root
  require_cmd docker

  if [ "$#" -eq 0 ] || [ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    usage
    exit 0
  fi

  if [ "$1" = "all" ]; then
    set -- daemon mysql timescaledb registry nginx
  fi

  local step
  for step in "$@"; do
    run_step "$step"
  done

  log "执行完成。"
}

main "$@"


