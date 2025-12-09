add_library(yw_controller STATIC
    resource_controller.cpp
    controller_factory.cpp
)

target_include_directories(yw_controller PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/src/controller
)

target_link_libraries(yw_controller PRIVATE
    yw_core
    yw_utils
)

