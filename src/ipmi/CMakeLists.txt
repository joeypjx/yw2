add_library(yw_ipmi STATIC
    ipmi_client.cpp
    ipmi_factory.cpp
)

find_package(Threads REQUIRED)

# 查找 libgcrypt 和 libgpg-error 库（跨平台支持）
# 优先使用 pkg-config（最可靠的方式）
find_program(PKG_CONFIG_EXECUTABLE pkg-config)

if (PKG_CONFIG_EXECUTABLE)
    # 使用 pkg-config 查找 libgcrypt
    execute_process(
        COMMAND ${PKG_CONFIG_EXECUTABLE} --libs libgcrypt
        OUTPUT_VARIABLE GCRYPT_PKG_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # 使用 pkg-config 查找 libgpg-error
    execute_process(
        COMMAND ${PKG_CONFIG_EXECUTABLE} --libs libgpg-error
        OUTPUT_VARIABLE GPG_ERROR_PKG_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# 如果 pkg-config 找到了库，直接使用
if (GCRYPT_PKG_LIBS)
    set(GCRYPT_LIBRARY ${GCRYPT_PKG_LIBS})
else()
    # 否则使用 find_library 在系统路径中查找
    # macOS: /opt/homebrew/lib, /usr/local/lib
    # Linux: /usr/lib, /usr/local/lib
    find_library(GCRYPT_LIBRARY
        NAMES gcrypt libgcrypt
        PATHS
            /opt/homebrew/lib      # macOS Homebrew (Apple Silicon)
            /usr/local/lib         # macOS Homebrew (Intel) / Linux 本地安装
            /usr/lib               # Linux 系统路径
            /usr/lib/x86_64-linux-gnu  # Debian/Ubuntu 64位
            /usr/lib64             # 某些 Linux 发行版
    )
endif()

if (GPG_ERROR_PKG_LIBS)
    set(GPG_ERROR_LIBRARY ${GPG_ERROR_PKG_LIBS})
else()
    find_library(GPG_ERROR_LIBRARY
        NAMES gpg-error libgpg-error
        PATHS
            /opt/homebrew/lib      # macOS Homebrew (Apple Silicon)
            /usr/local/lib         # macOS Homebrew (Intel) / Linux 本地安装
            /usr/lib               # Linux 系统路径
            /usr/lib/x86_64-linux-gnu  # Debian/Ubuntu 64位
            /usr/lib64             # 某些 Linux 发行版
    )
endif()

# 检查库是否找到，并提供平台相关的安装提示
if (NOT GCRYPT_LIBRARY)
    if (APPLE)
        set(INSTALL_CMD "brew install libgcrypt")
    elseif (UNIX)
        set(INSTALL_CMD "sudo apt-get install libgcrypt-dev  # Debian/Ubuntu\n    # 或: sudo yum install libgcrypt-devel  # CentOS/RHEL")
    else()
        set(INSTALL_CMD "请安装 libgcrypt 开发库")
    endif()
    message(FATAL_ERROR "libgcrypt not found. Please install it with:\n    ${INSTALL_CMD}")
endif()

if (NOT GPG_ERROR_LIBRARY)
    if (APPLE)
        set(INSTALL_CMD "brew install libgpg-error")
    elseif (UNIX)
        set(INSTALL_CMD "sudo apt-get install libgpg-error-dev  # Debian/Ubuntu\n    # 或: sudo yum install libgpg-error-devel  # CentOS/RHEL")
    else()
        set(INSTALL_CMD "请安装 libgpg-error 开发库")
    endif()
    message(FATAL_ERROR "libgpg-error not found. Please install it with:\n    ${INSTALL_CMD}")
endif()

target_include_directories(yw_ipmi PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(yw_ipmi PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ipmi
)

target_link_libraries(yw_ipmi PUBLIC
    ${FREEIPMI_LIBRARIES}
    ${GCRYPT_LIBRARY}
    ${GPG_ERROR_LIBRARY}
    Threads::Threads
    dl
)


