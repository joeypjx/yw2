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