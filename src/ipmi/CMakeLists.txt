add_library(yw_ipmi STATIC
    IpmiClient.cpp
    ipmi_factory.cpp
)

find_package(Threads REQUIRED)

target_include_directories(yw_ipmi PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_ipmi PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ipmi
)

target_link_libraries(yw_ipmi PUBLIC
    ${FREEIPMI_LIBRARIES}
    gcrypt
    gpg-error
    Threads::Threads
    dl
)


