cmake_minimum_required(VERSION 3.16)

# 设置编译器（必须在project()之前设置，且只能设置一次）
# 注意：如果编译器路径不存在，CMake会使用默认编译器，导致循环配置
# 如果不需要指定特定编译器，可以注释掉这两行，让CMake自动检测
if(EXISTS "/opt/homebrew/bin/gcc-15" AND EXISTS "/opt/homebrew/bin/g++-15")
    set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-15" CACHE PATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-15" CACHE PATH "C++ compiler" FORCE)
else()
    message(STATUS "Using default compiler (gcc-15/g++-15 not found)")
endif()

project(yw VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE "Debug")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -Wall -Wextra -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
else()
    # 默认模式：添加调试信息
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -ggdb3 -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -Wextra -ggdb3 -fno-omit-frame-pointer")
endif()

# 设置全局包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# libfreeipmi 配置（需要手动编译）
# ============================================================================
set(LIBFREEIPMI_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/.libs/libfreeipmi.a")
set(LIBFREEIPMI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/include")

# 检查 libfreeipmi 是否已手动编译
if(NOT EXISTS ${LIBFREEIPMI_LIB_PATH})
    message(FATAL_ERROR 
        "libfreeipmi 未找到，请先手动编译。\n"
        "库文件路径: ${LIBFREEIPMI_LIB_PATH}\n"
        "请参考 docs/manual_build_libfreeipmi.md 进行手动编译，或运行:\n"
        "  cd third_party/libfreeipmi && ./build_manual.sh"
    )
else()
    message(STATUS "找到 libfreeipmi: ${LIBFREEIPMI_LIB_PATH}")
endif()

# 设置libfreeipmi包含目录和库路径
include_directories(${LIBFREEIPMI_INCLUDE_DIR})
link_directories(${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/.libs)

# ============================================================================
# libpq (PostgreSQL客户端库) 配置（需要手动编译）
# ============================================================================
set(LIBPQ_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq/libpq.a")
set(LIBPQ_COMMON_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/postgres/src/common/libpgcommon.a")
set(LIBPQ_PORT_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/postgres/src/port/libpgport.a")
set(LIBPQ_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq")

# 检查 libpq 是否已手动编译
if(NOT EXISTS ${LIBPQ_LIB_PATH} OR NOT EXISTS ${LIBPQ_COMMON_LIB_PATH} OR NOT EXISTS ${LIBPQ_PORT_LIB_PATH})
    message(FATAL_ERROR 
        "libpq 未找到，请先手动编译。\n"
        "需要的库文件:\n"
        "  - ${LIBPQ_LIB_PATH}\n"
        "  - ${LIBPQ_COMMON_LIB_PATH}\n"
        "  - ${LIBPQ_PORT_LIB_PATH}\n"
        "请手动编译 PostgreSQL 的 libpq 部分。"
    )
else()
    message(STATUS "找到 libpq 库文件")
endif()

# 设置libpqxx构建选项（依赖libpq）
set(PostgreSQL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq;${CMAKE_SOURCE_DIR}/third_party/postgres/src/include")

# 静态链接libpq时，还需要libpgcommon和libpgport，否则会有pg_*符号未定义
set(PostgreSQL_LIBRARY
    "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq/libpq.a"
    "${CMAKE_SOURCE_DIR}/third_party/postgres/src/common/libpgcommon.a"
    "${CMAKE_SOURCE_DIR}/third_party/postgres/src/port/libpgport.a"
)
set(PostgreSQL_FOUND TRUE)
set(PostgreSQL_VERSION "REL_15_STABLE")
set(PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIR})
set(PostgreSQL_LIBRARIES ${PostgreSQL_LIBRARY})

# 设置libfreeipmi选项
set(FREEIPMI_INCLUDE_DIRS ${LIBFREEIPMI_INCLUDE_DIR})
set(FREEIPMI_LIBRARIES ${LIBFREEIPMI_LIB_PATH})
set(FREEIPMI_FOUND TRUE)

# 设置输出目录（必须在 add_subdirectory 之前设置，才能影响第三方库）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置libhv构建选项
set(BUILD_SHARED OFF CACHE BOOL "Build libhv shared library" FORCE)
set(BUILD_STATIC ON CACHE BOOL "Build libhv static library" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build libhv examples" FORCE)
set(BUILD_UNITTEST OFF CACHE BOOL "Build libhv unit tests" FORCE)

# 添加第三方库目录
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/libhv)
add_subdirectory(third_party/json)
add_subdirectory(third_party/eventpp)

# 先添加 libpqxx 子目录（依赖libpq）
set(SKIP_BUILD_TEST TRUE CACHE BOOL "Skip build test" FORCE)
add_subdirectory(third_party/libpqxx)

# 添加子目录
add_subdirectory(src)