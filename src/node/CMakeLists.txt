# 创建node模块库
add_library(yw_node STATIC
    node_cache.cpp
    node_manager.cpp
    node_factory.cpp
)

# 设置目标属性
set_target_properties(yw_node PROPERTIES
    OUTPUT_NAME "yw_node"
    POSITION_INDEPENDENT_CODE ON
)

# 链接依赖库 - 只需要链接yw_core，依赖会自动传递
target_link_libraries(yw_node PRIVATE
    yw_core          # 依赖core模块，spdlog和hv_static会自动传递
    yw_utils         # 依赖utils模块（MulticastScanner）
)

# 设置包含目录
target_include_directories(yw_node PUBLIC
    ${CMAKE_SOURCE_DIR}/include    # 公共头文件目录
)

target_include_directories(yw_node PRIVATE
    ${CMAKE_SOURCE_DIR}/src/node   # 模块内部头文件目录
    ${CMAKE_SOURCE_DIR}/src/core   # core模块头文件目录
    ${CMAKE_SOURCE_DIR}/src/utils  # utils 目录
)

# 设置编译定义
target_compile_definitions(yw_node PRIVATE
    YW_NODE_MODULE
)

# 安装规则（可选）
install(TARGETS yw_node
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(FILES 
    ${CMAKE_SOURCE_DIR}/include/yw/node.h
    DESTINATION include/yw
)


