// ============================================================================
// 文件功能描述：
// 数据库查询接口（DatabaseQueryInterface）的头文件，定义统一的数据库查询抽象。
// 主要功能包括：
// 1. 查询执行：执行SQL查询语句（支持无参数和参数化查询）
// 2. 连接池管理：使用PostgreSQL连接池管理数据库连接，提高并发性能
// 3. 结果转换：将PostgreSQL查询结果转换为QueryResult对象（包含列名和行数据）
// 4. 参数化查询：支持使用$1、$2等占位符的参数化查询，防止SQL注入
// 5. 错误处理：捕获和转换数据库异常，提供友好的错误信息
// 6. 接口抽象：定义DatabaseQueryInterface接口和PostgreSQLQueryInterface实现
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "utils/postgresql_connection_pool.h"

// 前向声明
namespace pqxx {
    class connection;
    class result;
}

namespace yw {
namespace alert {

// 查询结果行，包含列名到值的映射
struct QueryRow {
    std::unordered_map<std::string, std::string> columns;
    
    // 获取指定列的值
    // column: 列名
    // 返回: 列值，不存在时返回空字符串
    std::string getValue(const std::string& column) const {
        auto it = columns.find(column);
        return (it != columns.end()) ? it->second : "";
    }
    
    // 获取指定列的双精度浮点数值
    // column: 列名
    // 返回: 列值转换为double，不存在或转换失败时返回0.0
    double getDoubleValue(const std::string& column) const {
        auto it = columns.find(column);
        if (it != columns.end()) {
            try {
                return std::stod(it->second);
            } catch (...) {
                return 0.0;
            }
        }
        return 0.0;
    }
};

// 查询结果，包含多行数据
struct QueryResult {
    std::vector<QueryRow> rows;
    
    bool empty() const { return rows.empty(); }
    size_t size() const { return rows.size(); }
    const QueryRow& operator[](size_t index) const { return rows[index]; }
};

// 数据库查询接口抽象类
// 提供统一的数据库查询接口，支持无参数和参数化查询
class DatabaseQueryInterface {
public:
    virtual ~DatabaseQueryInterface() = default;
    
    // 执行无参数的SQL查询
    // sql: SQL查询语句
    // 返回: 查询结果
    virtual QueryResult executeQuery(const std::string& sql) = 0;
    
    // 执行参数化SQL查询
    // sql: SQL查询语句（使用$1, $2等占位符）
    // params: 参数列表
    // 返回: 查询结果
    virtual QueryResult executeQuery(const std::string& sql, 
                                   const std::vector<std::string>& params) = 0;
};


// PostgreSQL查询接口实现类
// 使用连接池管理数据库连接，提供参数化查询功能
class PostgreSQLQueryInterface : public DatabaseQueryInterface {
public:
    // 构造函数，初始化PostgreSQL连接池
    // conninfo: PostgreSQL连接字符串
    // minConnections: 连接池最小连接数（默认2）
    // maxConnections: 连接池最大连接数（默认10）
    explicit PostgreSQLQueryInterface(const std::string& conninfo,
                                      size_t minConnections = 2,
                                      size_t maxConnections = 10);
    ~PostgreSQLQueryInterface() override = default;
    
    QueryResult executeQuery(const std::string& sql) override;
    QueryResult executeQuery(const std::string& sql, 
                           const std::vector<std::string>& params) override;

private:
    std::unique_ptr<yw::utils::PostgreSQLConnectionPool> connectionPool_;
    
    // 使用指定的连接执行参数化SQL查询
    // conn: 数据库连接
    // sql: SQL查询语句（使用$1, $2等占位符）
    // params: 参数列表
    // 返回: 查询结果
    QueryResult executeQueryWithConnection(std::shared_ptr<pqxx::connection> conn,
                                          const std::string& sql,
                                          const std::vector<std::string>& params = {});
    
private:
    // 内部辅助函数：将 pqxx::result 转换为 QueryResult
    // result: pqxx查询结果
    // 返回: 转换后的QueryResult对象
    QueryResult convertResult(const pqxx::result& result);
};

} // namespace alert
} // namespace yw
