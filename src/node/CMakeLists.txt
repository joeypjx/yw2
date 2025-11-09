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
    yw_core
    yw_utils
)


