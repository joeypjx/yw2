// ============================================================================
// 文件功能描述：
// PostgreSQL连接池（PostgreSQLConnectionPool）的实现文件，提供数据库连接的复用和管理。
// 主要功能包括：
// 1. 连接池管理：维护最小和最大连接数，自动创建和销毁数据库连接
// 2. 连接获取：提供acquireConnection方法，从池中获取可用连接或创建新连接
// 3. 连接释放：提供releaseConnection方法，将连接归还到池中供其他请求使用
// 4. 连接有效性检查：自动检测连接是否有效，失效连接会被移除并重建
// 5. 线程安全：使用互斥锁和条件变量，支持多线程并发访问
// 6. RAII模式：提供ConnectionGuard类，自动管理连接的获取和释放
// 7. 资源管理：在析构时自动清理所有连接，确保资源正确释放
// ============================================================================

#include "utils/postgresql_connection_pool.h"
#include <pqxx/pqxx>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace yw {
namespace utils {

//=============================================================================
// PostgreSQLConnectionPool 实现
//=============================================================================

PostgreSQLConnectionPool::PostgreSQLConnectionPool(const std::string& conninfo,
                                                   size_t minConnections,
                                                   size_t maxConnections)
    : conninfo_(conninfo),
      minConnections_(minConnections),
      maxConnections_(maxConnections),
      activeConnections_(0),
      totalConnections_(0) {
    if (minConnections_ > maxConnections_) {
        throw std::invalid_argument("最小连接数不能大于最大连接数");
    }
    if (maxConnections_ == 0) {
        throw std::invalid_argument("最大连接数必须大于0");
    }
    
    initializePool();
    spdlog::debug("PostgreSQL连接池初始化完成: 最小={}, 最大={}", minConnections_, maxConnections_);
}

PostgreSQLConnectionPool::~PostgreSQLConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!availableConnections_.empty()) {
        availableConnections_.pop();
    }
    spdlog::debug("PostgreSQL连接池已销毁");
}

// 从连接池获取一个可用连接
// 如果池中有可用连接则直接返回，否则创建新连接（不超过最大连接数）
// 如果连接已失效则自动重建
std::shared_ptr<pqxx::connection> PostgreSQLConnectionPool::acquireConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 等待可用连接
    condition_.wait(lock, [this] {
        return !availableConnections_.empty() || totalConnections_ < maxConnections_;
    });
    
    std::shared_ptr<pqxx::connection> conn;
    
    if (!availableConnections_.empty()) {
        // 从池中获取连接
        conn = availableConnections_.front();
        availableConnections_.pop();
        
        // 检查连接是否有效
        if (!isConnectionValid(conn)) {
            spdlog::warn("连接已失效，创建新连接");
            conn = createConnection();
        }
    } else if (totalConnections_ < maxConnections_) {
        // 创建新连接
        conn = createConnection();
    }
    
    if (conn) {
        activeConnections_++;
    }
    
    return conn;
}

// 释放连接回连接池
// 如果连接有效则归还到池中，如果失效则从池中移除并减少总连接数
// 如果连接失效导致连接数低于最小连接数，会主动创建新连接补充
void PostgreSQLConnectionPool::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    if (!conn) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查连接是否有效
    if (isConnectionValid(conn)) {
        // 归还到池中
        availableConnections_.push(conn);
    } else {
        // 连接已失效，减少总连接数
        spdlog::warn("归还的连接已失效，从池中移除");
        if (totalConnections_ > 0) {
            totalConnections_--;
        }
        
        // 如果当前连接数低于最小连接数，主动补充连接
        size_t currentTotal = totalConnections_.load();
        if (currentTotal < minConnections_) {
            try {
                auto newConn = createConnection();
                if (newConn) {
                    availableConnections_.push(newConn);
                    spdlog::debug("连接失效后主动补充连接，当前总连接数: {}", totalConnections_.load());
                }
            } catch (const std::exception& e) {
                spdlog::error("连接失效后补充连接失败: {}", e.what());
            }
        }
    }
    
    if (activeConnections_ > 0) {
        activeConnections_--;
    }
    
    condition_.notify_one();
}

// 检查连接是否有效（通过执行简单查询测试）
// conn: 要检查的连接
// 返回: 连接有效返回true，无效返回false
bool PostgreSQLConnectionPool::isConnectionValid(std::shared_ptr<pqxx::connection> conn) const {
    if (!conn) {
        return false;
    }
    
    try {
        // 尝试执行一个简单查询来检查连接
        if (conn->is_open()) {
            pqxx::nontransaction ntx(*conn);
            ntx.exec("SELECT 1");
            return true;
        }
    } catch (const std::exception&) {
        // 连接已失效
        return false;
    }
    
    return false;
}

// 获取连接池中可用连接的数量
// 返回: 当前可用连接数
size_t PostgreSQLConnectionPool::getPoolSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return availableConnections_.size();
}

// 获取当前活跃连接数（正在使用的连接数）
// 返回: 当前活跃连接数
size_t PostgreSQLConnectionPool::getActiveConnections() const {
    return activeConnections_.load();
}

// 创建新的数据库连接并增加总连接数计数
// 返回: 新创建的连接，失败抛出异常
std::shared_ptr<pqxx::connection> PostgreSQLConnectionPool::createConnection() {
    try {
        auto conn = std::make_shared<pqxx::connection>(conninfo_);
        if (conn->is_open()) {
            totalConnections_++;
            spdlog::debug("创建新数据库连接，当前总连接数: {}", totalConnections_.load());
            return conn;
        } else {
            throw std::runtime_error("无法打开数据库连接");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("创建数据库连接失败: " + std::string(e.what()));
    }
}

// 初始化连接池，创建最小数量的连接
// 如果部分连接创建失败，会记录错误但继续尝试创建其他连接
void PostgreSQLConnectionPool::initializePool() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (size_t i = 0; i < minConnections_; ++i) {
        try {
            auto conn = createConnection();
            availableConnections_.push(conn);
        } catch (const std::exception& e) {
            spdlog::error("初始化连接池时创建连接失败: {}", e.what());
            // 继续尝试创建其他连接
        }
    }
    
    spdlog::debug("连接池初始化完成，创建了 {} 个连接", availableConnections_.size());
}

//=============================================================================
// ConnectionGuard 实现
//=============================================================================

// 连接守卫构造函数，自动从连接池获取连接
// pool: 连接池引用
// RAII模式：构造时获取连接，析构时自动释放
// 如果无法获取连接，抛出异常
ConnectionGuard::ConnectionGuard(PostgreSQLConnectionPool& pool)
    : pool_(pool) {
    conn_ = pool_.acquireConnection();
    if (!conn_) {
        throw std::runtime_error("无法从连接池获取连接");
    }
}

// 连接守卫析构函数，自动释放连接回连接池
ConnectionGuard::~ConnectionGuard() {
    if (conn_) {
        pool_.releaseConnection(conn_);
    }
}

// 检查当前持有的连接是否有效
// 返回: 连接有效返回true，无效返回false
bool ConnectionGuard::isValid() const {
    return conn_ && pool_.isConnectionValid(conn_);
}

} // namespace utils
} // namespace yw

