# 创建可执行文件
add_executable(yw_app main.cpp)

# 链接业务模块
# 注意：yw_web PUBLIC 链接了 yw_core 和 yw_alert，会自动传递
target_link_libraries(yw_app PRIVATE yw_node yw_monitor yw_web yw_bmc yw_controller)
# target_link_libraries(yw_app PRIVATE yw_ipmi)  # 暂时注释掉 IPMI 库

# 私有头文件目录
target_include_directories(yw_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/include
)