# alertv2 静态库 - DDD分层架构
add_library(yw_alertv2 STATIC
    # 领域层 (Domain Layer) - 核心业务逻辑
    domain/Alert.cpp
    domain/AlertRule.cpp
    domain/AlertRuleEvaluator.cpp
    
    # 应用层 (Application Layer) - 应用服务和用例
    application/AlertEngine.cpp
    
    # 基础设施层 (Infrastructure Layer) - 数据访问和外部服务
    infrastructure/AlertRepository.cpp
    infrastructure/AlertRuleRepository.cpp
    infrastructure/DatabaseQueryInterface.cpp
)

target_include_directories(yw_alertv2 PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}           # 根目录
    ${CMAKE_CURRENT_SOURCE_DIR}/domain    # 领域层
    ${CMAKE_CURRENT_SOURCE_DIR}/application  # 应用层
    ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure  # 基础设施层
)

target_include_directories(yw_alertv2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}           # 模块内部头文件目录
    ${CMAKE_CURRENT_SOURCE_DIR}/domain    # 领域层
    ${CMAKE_CURRENT_SOURCE_DIR}/application  # 应用层
    ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure  # 基础设施层
)

target_link_libraries(yw_alertv2 PRIVATE
    nlohmann_json::nlohmann_json
    pqxx
)
