git submodule add https://github.com/gabime/spdlog.git third_party/spdlog
git submodule update --init --recursive
// 确保 submodule 已初始化：git submodule update --init --recursive
git checkout v1.x
git add third_party/spdlog third_party/libhv
git commit -m "Pin spdlog to v1.12.0 and libhv to v1.3.3"
git submodule status

git reset --soft <commit-hash> # 撤销指定的commit ID
git reset --soft HEAD~1  # 撤销最后一次commit（保留修改）
git reset HEAD~1 # 撤销最后一次commit（保留修改，但不在暂存区）
git reset --hard HEAD~1 # 完全撤销最后一次commit（删除所有修改）
git revert HEAD  # 创建新的commit来撤销之前的commit, 使用 git revert 来避免强制推送

libpq(postgre)手动makefile编译

CC=/opt/homebrew/bin/gcc-15
CXX=/opt/homebrew/bin/g++-15
cmake .. -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-15
make -j8

docker run -d -p 12345:80 swaggerapi/swagger-editor:v5.0.0-alpha.113

// 

docker run -d --name timescaledb \
  -e POSTGRES_USER=postgres \
  -e POSTGRES_PASSWORD=HZ715Net \
  -e POSTGRES_DB=yw \
  -p 5432:5432 \
  -v /data/timescale_data:/var/lib/postgresql/data \
  timescale/timescaledb:2.21.2-pg16

docker cp *.sql timescaledb:/root
docker exec -it timescaledb /bin/bash

???
psql -U postgres
 CREATE DATABASE yw;
 quit

psql -U postgres -d yw -f timescaledb_setup.sql
psql -U postgres -d yw -f bmc_timescaledb_setup.sql
psql -U postgres -d yw -f alert_rule_setup.sql
psql -U postgres -d yw -f alert_event_setup_basic.sql


//

docker load -i tdengine.tar
docker run -d -p 6030:6030 -p 6041:6041 -p 6043:6043 -p 6044-6049:6044-6049 -p 6044-6045:6044-6045/udp -p 6060:6060 tdengine/tdengine:latest

test HZ715Net resource

