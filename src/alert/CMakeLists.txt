# alert 静态库 - DDD分层架构
add_library(yw_alert STATIC
    # 模块接口层 - 公共接口实现
    alert_module.cpp
    
    # 领域层 (Domain Layer) - 核心业务逻辑
    domain/alert_event.cpp
    domain/alert_rule.cpp
    domain/alert_rule_evaluator.cpp
    
    # 应用层 (Application Layer) - 应用服务和用例
    application/alert_engine.cpp
    application/alert_creation_factory.cpp
    application/alert_rule_service.cpp
    application/alert_query_service.cpp
    
    # 基础设施层 (Infrastructure Layer) - 数据访问和外部服务
    infrastructure/alert_event_repository.cpp
    infrastructure/alert_rule_repository.cpp
    infrastructure/database_query_interface.cpp
)

target_include_directories(yw_alert PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}           # 根目录
    ${CMAKE_CURRENT_SOURCE_DIR}/domain    # 领域层
    ${CMAKE_CURRENT_SOURCE_DIR}/application  # 应用层
    ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure  # 基础设施层
)

target_include_directories(yw_alert PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}           # 模块内部头文件目录
    ${CMAKE_CURRENT_SOURCE_DIR}/domain    # 领域层
    ${CMAKE_CURRENT_SOURCE_DIR}/application  # 应用层
    ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure  # 基础设施层
)

target_link_libraries(yw_alert PRIVATE
    yw_utils
    pqxx
    # spdlog, nlohmann_json 通过 yw_utils 的 PUBLIC 链接自动传递
)
