# 创建可执行文件
add_executable(yw_app main.cpp)

# 链接第三方库
target_link_libraries(yw_app PRIVATE
    spdlog::spdlog_header_only
    hv_static
    nlohmann_json::nlohmann_json
)

# 链接业务模块
# 注意：yw_web PUBLIC 链接了 yw_alert，会自动传递
target_link_libraries(yw_app PRIVATE 
    yw_core
    yw_node 
    yw_monitor 
    yw_web 
    yw_bmc 
    yw_controller
    yw_utils
)
# target_link_libraries(yw_app PRIVATE yw_ipmi)  # 暂时注释掉 IPMI 库

# 私有头文件目录
target_include_directories(yw_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/include
)