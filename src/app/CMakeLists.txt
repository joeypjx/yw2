# 创建可执行文件
add_executable(yw_app main.cpp)

# 链接核心库（会自动包含其PUBLIC头文件目录）
# 注意：yw_core 已经 PUBLIC 链接了 spdlog::spdlog，会自动传递，无需重复链接
target_link_libraries(yw_app PRIVATE yw_core yw_node yw_monitor yw_web yw_bmc yw_alert)
# target_link_libraries(yw_app PRIVATE yw_ipmi)  # 暂时注释掉 IPMI 库

# 私有头文件目录
target_include_directories(yw_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/include
)