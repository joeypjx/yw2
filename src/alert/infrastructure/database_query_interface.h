#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

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
    explicit PostgreSQLQueryInterface(const std::string& conninfo);
    ~PostgreSQLQueryInterface() override = default;
    
    QueryResult executeQuery(const std::string& sql) override;
    QueryResult executeQuery(const std::string& sql, 
                           const std::vector<std::string>& params) override;

private:
    std::string conninfo_;
};

} // namespace alert
} // namespace yw
