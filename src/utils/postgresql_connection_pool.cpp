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
    }
    
    if (activeConnections_ > 0) {
        activeConnections_--;
    }
    
    condition_.notify_one();
}

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

size_t PostgreSQLConnectionPool::getPoolSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return availableConnections_.size();
}

size_t PostgreSQLConnectionPool::getActiveConnections() const {
    return activeConnections_.load();
}

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

ConnectionGuard::ConnectionGuard(PostgreSQLConnectionPool& pool)
    : pool_(pool) {
    conn_ = pool_.acquireConnection();
    if (!conn_) {
        throw std::runtime_error("无法从连接池获取连接");
    }
}

ConnectionGuard::~ConnectionGuard() {
    if (conn_) {
        pool_.releaseConnection(conn_);
    }
}

bool ConnectionGuard::isValid() const {
    return conn_ && pool_.isConnectionValid(conn_);
}

} // namespace utils
} // namespace yw

