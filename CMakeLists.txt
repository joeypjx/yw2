cmake_minimum_required(VERSION 3.16)
project(yw VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置全局包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 设置 libpq 构建输出目录
set(LIBPQ_BUILD_DIR "${CMAKE_BINARY_DIR}/third_party/postgres")
set(LIBPQ_LIBRARY_PATH "${LIBPQ_BUILD_DIR}/libpq.a")
set(LIBPQ_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/third_party/postgres/src/interfaces/libpq")

# 调用外部 Makefile 构建 libpq
add_custom_target(libpq_build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/postgres
    # 创建构建目录
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBPQ_BUILD_DIR}
    # 配置 PostgreSQL，指定额外的头文件搜索路径
    COMMAND ${CMAKE_COMMAND} -E env 
            CPPFLAGS=-I${CMAKE_SOURCE_DIR}/third_party/postgres/src/backend
            ./configure --without-readline --without-zlib --without-openssl
    # 只构建 libpq 及其依赖
    COMMAND make -C src/include all
    COMMAND make -C src/port all  
    COMMAND make -C src/common all
    COMMAND make -C src/interfaces/libpq libpq.a
    # 复制到构建目录
    COMMAND ${CMAKE_COMMAND} -E copy src/interfaces/libpq/libpq.a ${LIBPQ_LIBRARY_PATH}
    # 清理
    COMMAND make clean
    COMMENT "Building libpq with proper configure environment"
    BYPRODUCTS ${LIBPQ_LIBRARY_PATH}
)

set(PostgreSQL_INCLUDE_DIR "${LIBPQ_INCLUDE_PATH};${CMAKE_SOURCE_DIR}/third_party/postgres/src/include")
set(PostgreSQL_LIBRARY "${LIBPQ_LIBRARY_PATH}")
set(PostgreSQL_FOUND TRUE)
set(PostgreSQL_VERSION "REL_15_STABLE")
set(PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIR})
set(PostgreSQL_LIBRARIES ${PostgreSQL_LIBRARY})

# 设置libhv构建选项
set(BUILD_SHARED OFF CACHE BOOL "Build libhv shared library" FORCE)
set(BUILD_STATIC ON CACHE BOOL "Build libhv static library" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build libhv examples" FORCE)
set(BUILD_UNITTEST OFF CACHE BOOL "Build libhv unit tests" FORCE)

# 添加第三方库目录
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/libhv)

# 先添加 libpqxx 子目录
set(SKIP_BUILD_TEST TRUE CACHE BOOL "Skip build test" FORCE)
add_subdirectory(third_party/libpqxx)

# 然后设置依赖关系
add_dependencies(pqxx libpq_build)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加子目录
add_subdirectory(src)