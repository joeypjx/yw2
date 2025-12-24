add_library(yw_node STATIC
    node_cache.cpp
    node_manager.cpp
    node_factory.cpp
)

target_include_directories(yw_node PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_node PRIVATE
    ${CMAKE_SOURCE_DIR}/src/node
)

target_link_libraries(yw_node PRIVATE
    yw_utils
    # spdlog, hv_static, nlohmann_json 通过 yw_utils 的 PUBLIC 链接自动传递
    # 注意：虽然源代码直接使用了这些库，但通过 PUBLIC 链接可以自动传递依赖
)


