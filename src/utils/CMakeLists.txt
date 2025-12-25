add_library(yw_utils STATIC
    multicast_scanner.cpp
    json_config.cpp
    duration_utils.cpp
    time_utils.cpp
    postgresql_connection_pool.cpp
    ip_address_utils.cpp
    response_builder.cpp
)

# 公开头文件目录 - 通过 include/utils 访问
target_include_directories(yw_utils PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 私有头文件目录 - 仅内部使用
target_include_directories(yw_utils PRIVATE
    ${CMAKE_SOURCE_DIR}/src/utils
)

target_link_libraries(yw_utils PUBLIC
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    hv_static
    pqxx
)


