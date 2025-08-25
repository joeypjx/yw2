add_library(yw_alert STATIC
    AlertManager.cpp
    AlertServices.cpp
    alert_factory.cpp
)

target_include_directories(yw_alert PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(yw_alert
    spdlog
    nlohmann_json
    pqxx
)


