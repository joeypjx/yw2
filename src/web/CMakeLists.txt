add_library(yw_web STATIC
    web_controller.cpp
    web_factory.cpp
    mapper/node_mapper.cpp
    routes/node_routes.cpp
    routes/metrics_routes.cpp
    routes/bmc_routes.cpp
    routes/alert_routes.cpp
    services/alert_pusher.cpp
)

target_include_directories(yw_web PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_web PRIVATE
    ${CMAKE_SOURCE_DIR}/src/web
    ${CMAKE_SOURCE_DIR}/third_party/json/include  # 优先使用third_party/json
)

# PUBLIC 链接 - 依赖会传递给链接 yw_web 的目标
target_link_libraries(yw_web PUBLIC
    yw_alert
)

# PRIVATE 链接 - 仅内部使用
target_link_libraries(yw_web PRIVATE
    spdlog::spdlog_header_only
    hv_static
    nlohmann_json::nlohmann_json
    yw_utils
)


