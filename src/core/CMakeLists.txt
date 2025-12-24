# 创建核心库
add_library(yw_core STATIC
    app_context.cpp
)

# 链接第三方库 - 使用PRIVATE，因为各模块通过 yw_utils 传递依赖
target_link_libraries(yw_core PRIVATE 
    yw_utils
    # spdlog, hv_static, nlohmann_json 通过 yw_utils 的 PUBLIC 链接自动传递
)

# 公开头文件目录 - 其他链接此库的目标会自动包含
target_include_directories(yw_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 私有头文件目录 - 仅此库内部使用
target_include_directories(yw_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
)