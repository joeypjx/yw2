add_library(yw_web STATIC
    web_controller.cpp
    web_factory.cpp
    mapper/NodeMapper.cpp
    routes/NodeRoutes.cpp
    routes/MetricsRoutes.cpp
    routes/BMCRoutes.cpp
    routes/AlertV2Routes.cpp
    AlertPusher.cpp
)

target_include_directories(yw_web PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_web PRIVATE
    ${CMAKE_SOURCE_DIR}/src/web
    ${CMAKE_SOURCE_DIR}/third_party/json/include  # 优先使用third_party/json
)

target_link_libraries(yw_web PRIVATE
    yw_core
    yw_alertv2
    hv_static
    nlohmann_json::nlohmann_json
)


