# 创建核心库
add_library(yw_core STATIC
    app_context.cpp
)

# 链接第三方库 - 使用PRIVATE，因为各模块直接链接第三方库
target_link_libraries(yw_core PRIVATE 
    spdlog::spdlog_header_only 
    hv_static 
    nlohmann_json::nlohmann_json
    yw_utils
)

# 公开头文件目录 - 其他链接此库的目标会自动包含
target_include_directories(yw_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 私有头文件目录 - 仅此库内部使用
target_include_directories(yw_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
)