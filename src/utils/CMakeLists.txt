add_library(yw_utils STATIC
    MulticastScanner.cpp
    JsonConfig.cpp
    DurationUtils.cpp
)

target_include_directories(yw_utils PUBLIC
    ${CMAKE_SOURCE_DIR}/src/utils
)

target_link_libraries(yw_utils PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)


