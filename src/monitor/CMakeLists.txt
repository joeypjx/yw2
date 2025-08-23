add_library(yw_monitor STATIC
    monitor_manager.cpp
    monitor_factory.cpp
    resource_repository.cpp
)

target_include_directories(yw_monitor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_monitor PRIVATE
    ${CMAKE_SOURCE_DIR}/src/monitor   # 模块内部头文件目录
    ${CMAKE_SOURCE_DIR}/src/core   # core模块头文件目录
)

target_link_libraries(yw_monitor PRIVATE
    yw_core
    pqxx
)

