add_library(yw_alert STATIC
    AlertManager.cpp
    AlertServices.cpp
    alert_factory.cpp
    DatabaseEventRepository.cpp
    DatabaseRuleRepository.cpp
    AlertPusher.cpp
)

target_include_directories(yw_alert PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/eventpp/include
)

target_link_libraries(yw_alert PRIVATE
    yw_core
    hv_static
    nlohmann_json::nlohmann_json
    pqxx
)