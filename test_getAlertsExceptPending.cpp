#include <iostream>
#include <memory>
#include "../../src/alertv2/infrastructure/AlertRepository.h"
#include "../../src/alertv2/infrastructure/DatabaseQueryInterface.h"
#include "../../src/alertv2/domain/Alert.h"

using namespace yw::alertv2;

int main() {
    try {
        // 创建数据库连接（需要根据实际配置调整）
        auto dbInterface = std::make_shared<DatabaseQueryInterface>("postgres://postgres:HZ715Net@localhost:5432/yw");
        
        // 创建 AlertRepository
        DatabaseAlertRepository alertRepo(dbInterface);
        
        // 测试新方法
        std::cout << "测试 getAlertsExceptPending() 方法..." << std::endl;
        
        auto alerts = alertRepo.getAlertsExceptPending();
        
        std::cout << "找到 " << alerts.size() << " 个非Pending状态的告警:" << std::endl;
        
        for (const auto& alert : alerts) {
            std::cout << "  - ID: " << alert.getId() 
                      << ", Fingerprint: " << alert.getFingerprint()
                      << ", Status: ";
            
            switch (alert.getStatus()) {
                case AlertStatus::Pending:
                    std::cout << "Pending";
                    break;
                case AlertStatus::Firing:
                    std::cout << "Firing";
                    break;
                case AlertStatus::Resolved:
                    std::cout << "Resolved";
                    break;
            }
            
            std::cout << ", Created: " << alert.getCreatedAt() << std::endl;
        }
        
        // 验证结果：不应该包含任何Pending状态的告警
        bool hasPending = false;
        for (const auto& alert : alerts) {
            if (alert.getStatus() == AlertStatus::Pending) {
                hasPending = true;
                break;
            }
        }
        
        if (hasPending) {
            std::cerr << "错误: 结果中包含Pending状态的告警！" << std::endl;
            return 1;
        } else {
            std::cout << "✅ 验证通过: 结果中不包含Pending状态的告警" << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
