add_library(yw_web STATIC
    web_controller.cpp
    web_factory.cpp
    mapper/AlertMapper.cpp
    mapper/NodeMapper.cpp
    routes/NodeRoutes.cpp
    routes/MetricsRoutes.cpp
    routes/BMCRoutes.cpp
    routes/AlertRoutes.cpp
    AlertPusher.cpp
)

target_include_directories(yw_web PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_web PRIVATE
    ${CMAKE_SOURCE_DIR}/src/web
)

target_link_libraries(yw_web PRIVATE
    yw_core
    hv_static
    nlohmann_json::nlohmann_json
)


