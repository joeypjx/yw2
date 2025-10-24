# 创建可执行文件
add_executable(yw_app main.cpp)

# 链接核心库（会自动包含其PUBLIC头文件目录）
target_link_libraries(yw_app PRIVATE yw_core yw_node yw_monitor yw_web yw_bmc yw_alertv2)
# target_link_libraries(yw_app PRIVATE yw_ipmi)  # 暂时注释掉 IPMI 库

# 链接第三方库
target_link_libraries(yw_app PRIVATE spdlog::spdlog)

# 私有头文件目录
target_include_directories(yw_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/include
)