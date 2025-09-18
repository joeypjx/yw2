cmake_minimum_required(VERSION 3.16)
project(yw VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器
# set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-15")
# set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-15")
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
## libfreeipmi
include_directories(${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/include)
link_directories(${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/.libs)

# 设置 libpq 构建输出目录
# set(LIBPQ_BUILD_DIR "${CMAKE_BINARY_DIR}/third_party/postgres")
# set(LIBPQ_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq/libpq.a")
# set(LIBPQ_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq")

# 调用外部 Makefile 构建 libpq
# add_custom_target(libpq_build
#     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/postgres
#     # 创建构建目录
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBPQ_BUILD_DIR}
#     # 配置 PostgreSQL，指定额外的头文件搜索路径
#     COMMAND ${CMAKE_COMMAND} -E env 
#             CPPFLAGS="-I${CMAKE_SOURCE_DIR}/third_party/postgres/src/backend -I${CMAKE_SOURCE_DIR}/third_party/postgres/src/include"
#             ./configure --without-readline --without-zlib --without-openssl
#     # 只构建 libpq 及其依赖
#     COMMAND make -C src/include all
#     COMMAND make -C src/port all  
#     COMMAND make -C src/common all
#     COMMAND make -C src/interfaces/libpq -I${CMAKE_SOURCE_DIR}/third_party/postgres/src/backend -I${CMAKE_SOURCE_DIR}/third_party/postgres/src/include libpq.a
#     # 复制到构建目录
#     COMMAND ${CMAKE_COMMAND} -E copy src/interfaces/libpq/libpq.a ${LIBPQ_LIBRARY_PATH}
#     # 清理
#     COMMAND make distclean
#     COMMENT "Building libpq with proper configure environment"
#     BYPRODUCTS ${LIBPQ_LIBRARY_PATH}
# )

# 设置libpqxx构建选项
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
set(FREEIPMI_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/include")
set(FREEIPMI_LIBRARIES
    "${CMAKE_SOURCE_DIR}/third_party/libfreeipmi/libfreeipmi/.libs/libfreeipmi.a"
)
set(FREEIPMI_FOUND TRUE)

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

# 先添加 libpqxx 子目录
set(SKIP_BUILD_TEST TRUE CACHE BOOL "Skip build test" FORCE)
add_subdirectory(third_party/libpqxx)

# 然后设置依赖关系
# add_dependencies(pqxx libpq_build)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加子目录
add_subdirectory(src)