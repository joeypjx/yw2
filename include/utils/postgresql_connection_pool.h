// ============================================================================
// 文件功能描述：
// PostgreSQL连接池（PostgreSQLConnectionPool）的头文件，定义数据库连接池的接口。
// 主要功能包括：
// 1. 连接池管理：提供连接池的创建、初始化和销毁功能
// 2. 连接获取：提供acquireConnection方法，从池中获取可用连接或创建新连接
// 3. 连接释放：提供releaseConnection方法，将连接归还到池中
// 4. 连接有效性检查：提供isConnectionValid方法，检查连接是否有效
// 5. RAII模式：提供ConnectionGuard类，自动管理连接的获取和释放
// 6. 线程安全：使用互斥锁和条件变量，支持多线程并发访问
// ============================================================================

#pragma once

#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

// 前向声明
namespace pqxx {
    class connection;
}

namespace yw {
namespace utils {

/**
 * @brief PostgreSQL 连接池类
 * 
 * 管理 PostgreSQL 数据库连接的池，支持连接复用和线程安全访问
 */
class PostgreSQLConnectionPool {
public:
    /**
     * @brief 构造函数
     * @param conninfo 数据库连接信息
     * @param minConnections 最小连接数（默认2）
     * @param maxConnections 最大连接数（默认10）
     */
    explicit PostgreSQLConnectionPool(const std::string& conninfo,
                                     size_t minConnections = 2,
                                     size_t maxConnections = 10);
    
    /**
     * @brief 析构函数，关闭所有连接
     */
    ~PostgreSQLConnectionPool();
    
    /**
     * @brief 获取一个连接（RAII 方式）
     * @return 连接指针，使用完毕后自动归还到池中
     */
    std::shared_ptr<pqxx::connection> acquireConnection();
    
    /**
     * @brief 归还连接到池中
     * @param conn 要归还的连接
     */
    void releaseConnection(std::shared_ptr<pqxx::connection> conn);
    
    /**
     * @brief 检查连接是否有效
     * @param conn 要检查的连接
     * @return 连接是否有效
     */
    bool isConnectionValid(std::shared_ptr<pqxx::connection> conn) const;
    
    /**
     * @brief 获取当前池中的连接数
     */
    size_t getPoolSize() const;
    
    /**
     * @brief 获取当前正在使用的连接数
     */
    size_t getActiveConnections() const;

private:
    std::string conninfo_;
    size_t minConnections_;
    size_t maxConnections_;
    
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::shared_ptr<pqxx::connection>> availableConnections_;
    std::atomic<size_t> activeConnections_;
    std::atomic<size_t> totalConnections_;
    
    /**
     * @brief 创建新连接
     */
    std::shared_ptr<pqxx::connection> createConnection();
    
    /**
     * @brief 初始化连接池（创建最小连接数）
     */
    void initializePool();
};

/**
 * @brief 连接守卫类（RAII）
 * 
 * 自动管理连接的获取和释放
 */
class ConnectionGuard {
public:
    explicit ConnectionGuard(PostgreSQLConnectionPool& pool);
    ~ConnectionGuard();
    
    // 禁止拷贝
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    
    // 允许移动
    ConnectionGuard(ConnectionGuard&&) noexcept = default;
    ConnectionGuard& operator=(ConnectionGuard&&) noexcept = default;
    
    /**
     * @brief 获取连接指针
     */
    std::shared_ptr<pqxx::connection> get() const { return conn_; }
    
    /**
     * @brief 操作符重载，直接使用连接
     */
    std::shared_ptr<pqxx::connection> operator->() const { return conn_; }
    
    /**
     * @brief 检查连接是否有效
     */
    bool isValid() const;

private:
    PostgreSQLConnectionPool& pool_;
    std::shared_ptr<pqxx::connection> conn_;
};

} // namespace utils
} // namespace yw

