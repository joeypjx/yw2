#include "DatabaseQueryInterface.h"
#include <pqxx/pqxx>
#include <stdexcept>

namespace yw {
namespace alertv2 {

PostgreSQLQueryInterface::PostgreSQLQueryInterface(const std::string& conninfo)
    : conninfo_(conninfo) {}

QueryResult PostgreSQLQueryInterface::executeQuery(const std::string& sql) {
    try {
        pqxx::connection c(conninfo_);
        pqxx::work tx{c};
        pqxx::result result = tx.exec(sql);
        
        QueryResult queryResult;
        for (const auto& row : result) {
            QueryRow queryRow;
            for (size_t i = 0; i < row.size(); ++i) {
                std::string columnName = result.column_name(i);
                std::string value = row[i].is_null() ? "" : row[i].as<std::string>();
                queryRow.columns[columnName] = value;
            }
            queryResult.rows.push_back(queryRow);
        }
        
        tx.commit();
        return queryResult;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database query failed: " + std::string(e.what()));
    }
}

QueryResult PostgreSQLQueryInterface::executeQuery(const std::string& sql, 
                                                   const std::vector<std::string>& params) {
    try {
        pqxx::connection c(conninfo_);
        pqxx::work tx{c};
        
        // 构建参数化查询
        std::string paramSql = sql;
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "$" + std::to_string(i + 1);
            size_t pos = paramSql.find(placeholder);
            if (pos != std::string::npos) {
                paramSql.replace(pos, placeholder.length(), "'" + params[i] + "'");
            }
        }
        
        pqxx::result result = tx.exec(paramSql);
        
        QueryResult queryResult;
        for (const auto& row : result) {
            QueryRow queryRow;
            for (size_t i = 0; i < row.size(); ++i) {
                std::string columnName = result.column_name(i);
                std::string value = row[i].is_null() ? "" : row[i].as<std::string>();
                queryRow.columns[columnName] = value;
            }
            queryResult.rows.push_back(queryRow);
        }
        
        tx.commit();
        return queryResult;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database query failed: " + std::string(e.what()));
    }
}

} // namespace alertv2
} // namespace yw
