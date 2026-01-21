# 导入镜像
cd docker-images
for file in *.tar; do
    docker load -i "$file"
done
cd ..

# 删除当前运行容器
docker rm -f mariadb
docker rm -f timescaledb
docker rm -f nginx
docker rm -f registry
docker rm -f adminer

# 创建自定义网络
docker network create my-network

# 启动 MariaDB
docker run -d \
  --name mariadb \
  --network my-network \
  --restart always \
  -e TZ=Asia/Shanghai \
  -e MARIADB_ROOT_PASSWORD=HZ715Net \
  -e MARIADB_DATABASE=soft_factory \
  -e MARIADB_USER=soft_factory \
  -e MARIADB_PASSWORD=zhaochen123 \
  -p 3306:3306 \
  -v $(pwd)/mariadb/data:/var/lib/mysql \
  -v $(pwd)/mariadb/conf.d:/etc/mysql/conf.d \
  mariadb:10.11

# 启动 TimescaleDB
docker run -d \
  --name timescaledb \
  --network my-network \
  --restart always \
  -e POSTGRES_USER=postgres \
  -e POSTGRES_PASSWORD=HZ715Net \
  -e POSTGRES_DB=yw2 \
  -p 5432:5432 \
  -v $(pwd)/timescaledb/data:/var/lib/postgresql/data \
  -v $(pwd)/timescaledb/init-scripts:/docker-entrypoint-initdb.d \
  timescale/timescaledb:2.21.2-pg16

# 启动 Nginx
docker run -d \
  --name nginx \
  --network my-network \
  --restart always \
  -p 80:80 \
  -p 443:443 \
  -v $(pwd)/nginx/conf.d:/etc/nginx/conf.d \
  -v $(pwd)/nginx/html:/usr/share/nginx/html \
  -v $(pwd)/nginx/logs:/var/log/nginx \
  nginx:1.26

# 启动 Registry
docker run -d \
  --name registry \
  --network my-network \
  --restart always \
  -p 5000:5000 \
  -v $(pwd)/registry/data:/var/lib/registry \
  registry:2.8

# 启动 Adminer（数据库管理）
docker run -d \
  --name adminer \
  --network my-network \
  --restart always \
  -p 8089:8080 \
  adminer:4.8.1
