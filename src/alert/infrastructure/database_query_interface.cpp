#include "database_query_interface.h"
#include "utils/postgresql_connection_pool.h"
#include <pqxx/pqxx>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace yw {
namespace alert {

//=============================================================================
// PostgreSQLQueryInterface 实现
//=============================================================================

// PostgreSQL查询接口构造函数
// conninfo: PostgreSQL连接字符串
// minConnections: 连接池最小连接数
// maxConnections: 连接池最大连接数
PostgreSQLQueryInterface::PostgreSQLQueryInterface(const std::string& conninfo,
                                                   size_t minConnections,
                                                   size_t maxConnections)
    : connectionPool_(std::make_unique<yw::utils::PostgreSQLConnectionPool>(conninfo, minConnections, maxConnections)) {
}

// 执行无参数的SQL查询
// sql: SQL查询语句
// 返回: 查询结果
QueryResult PostgreSQLQueryInterface::executeQuery(const std::string& sql) {
    return executeQuery(sql, {});
}

// 执行参数化SQL查询
// sql: SQL查询语句（使用$1, $2等占位符）
// params: 参数列表
// 返回: 查询结果，失败抛出异常
QueryResult PostgreSQLQueryInterface::executeQuery(const std::string& sql, 
                                                   const std::vector<std::string>& params) {
    // 获取连接
    yw::utils::ConnectionGuard guard(*connectionPool_);
    auto conn = guard.get();
    
    if (!conn) {
        throw std::runtime_error("无法从连接池获取连接");
    }
    
    return executeQueryWithConnection(conn, sql, params);
}

// 使用指定的连接执行参数化SQL查询
// conn: 数据库连接
// sql: SQL查询语句（使用$1, $2等占位符）
// params: 参数列表
// 返回: 查询结果，失败抛出异常
QueryResult PostgreSQLQueryInterface::executeQueryWithConnection(
    std::shared_ptr<pqxx::connection> conn,
    const std::string& sql,
    const std::vector<std::string>& params) {
    
    try {
        pqxx::work tx{*conn};
        pqxx::result result;
        
        if (params.empty()) {
            // 无参数查询
            result = tx.exec(sql);
        } else {
            // 使用 pqxx 的参数化查询
            // 注意：pqxx 的参数化查询使用 $1, $2 等占位符
            // 我们需要将参数转换为 pqxx 可以使用的格式
            
            // 构建参数数组
            std::vector<const char*> paramPtrs;
            std::vector<std::string> paramStrs; // 保持参数的生命周期
            
            for (const auto& param : params) {
                paramStrs.push_back(param);
                paramPtrs.push_back(paramStrs.back().c_str());
            }
            
            // 使用 prepare 和参数化查询
            // 注意：pqxx 的参数化查询需要预先准备语句
            // 这里我们使用一个简化的方法：直接构建参数化查询字符串
            // 但使用 pqxx 的转义功能来防止 SQL 注入
            
            // 构建参数化查询字符串
        std::string paramSql = sql;
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "$" + std::to_string(i + 1);
            size_t pos = paramSql.find(placeholder);
            if (pos != std::string::npos) {
                    // 使用 pqxx 的转义功能
                    std::string escaped = tx.esc(params[i]);
                    paramSql.replace(pos, placeholder.length(), "'" + escaped + "'");
            }
        }
        
            result = tx.exec(paramSql);
        }
        
        tx.commit();
        
        return convertResult(result);
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database query failed: " + std::string(e.what()));
    }
}

// 将pqxx查询结果转换为QueryResult对象
// result: pqxx查询结果
// 返回: 转换后的QueryResult对象
QueryResult PostgreSQLQueryInterface::convertResult(const pqxx::result& result) {
    QueryResult queryResult;
    
    // 缓存列名（避免重复调用 column_name）
    std::vector<std::string> columnNames;
    if (!result.empty()) {
        for (size_t i = 0; i < result.columns(); ++i) {
            columnNames.push_back(result.column_name(i));
        }
    }
    
    for (const auto& row : result) {
        QueryRow queryRow;
        for (size_t i = 0; i < row.size() && i < columnNames.size(); ++i) {
            std::string value = row[i].is_null() ? "" : row[i].as<std::string>();
            queryRow.columns[columnNames[i]] = value;
        }
        queryResult.rows.push_back(queryRow);
    }
    
    return queryResult;
}

} // namespace alert
} // namespace yw
