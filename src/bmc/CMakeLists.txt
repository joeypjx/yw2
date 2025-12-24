add_library(yw_bmc STATIC
    bmc_listener.cpp
    bmc_repository.cpp
    bmc_factory.cpp
    bmc_cache.cpp
)

target_include_directories(yw_bmc PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_bmc PRIVATE
    ${CMAKE_SOURCE_DIR}/src/bmc
)

target_link_libraries(yw_bmc PRIVATE
    yw_utils
    pqxx
    # spdlog, nlohmann_json 通过 yw_utils 的 PUBLIC 链接自动传递
)


