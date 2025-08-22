# 创建可执行文件
add_executable(yw_app main.cpp)

# 链接核心库（会自动包含其PUBLIC头文件目录）
target_link_libraries(yw_app PRIVATE yw_core)

# 链接第三方库
target_link_libraries(yw_app PRIVATE spdlog::spdlog)

# 私有头文件目录
target_include_directories(yw_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/app
)