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

// 查询结果行
struct QueryRow {
    std::unordered_map<std::string, std::string> columns;
    
    std::string getValue(const std::string& column) const {
        auto it = columns.find(column);
        return (it != columns.end()) ? it->second : "";
    }
    
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

// 查询结果
struct QueryResult {
    std::vector<QueryRow> rows;
    
    bool empty() const { return rows.empty(); }
    size_t size() const { return rows.size(); }
    const QueryRow& operator[](size_t index) const { return rows[index]; }
};

class DatabaseQueryInterface {
public:
    virtual ~DatabaseQueryInterface() = default;
    
    virtual QueryResult executeQuery(const std::string& sql) = 0;
    virtual QueryResult executeQuery(const std::string& sql, 
                                   const std::vector<std::string>& params) = 0;
};


class PostgreSQLQueryInterface : public DatabaseQueryInterface {
public:
    /**
     * @brief 构造函数
     * @param conninfo 数据库连接信息
     * @param minConnections 连接池最小连接数（默认2）
     * @param maxConnections 连接池最大连接数（默认10）
     */
    explicit PostgreSQLQueryInterface(const std::string& conninfo,
                                      size_t minConnections = 2,
                                      size_t maxConnections = 10);
    ~PostgreSQLQueryInterface() override = default;
    
    QueryResult executeQuery(const std::string& sql) override;
    QueryResult executeQuery(const std::string& sql, 
                           const std::vector<std::string>& params) override;

private:
    std::unique_ptr<yw::utils::PostgreSQLConnectionPool> connectionPool_;
    
    /**
     * @brief 使用真正的参数化查询执行 SQL
     */
    QueryResult executeQueryWithConnection(std::shared_ptr<pqxx::connection> conn,
                                          const std::string& sql,
                                          const std::vector<std::string>& params = {});
    
private:
    // 内部辅助函数：将 pqxx::result 转换为 QueryResult
    // 在实现文件中定义，使用 pqxx::result 类型
    QueryResult convertResult(const pqxx::result& result);
};

} // namespace alert
} // namespace yw
