#pragma once

#include <vector>
#include <string>
#include <mutex>

// 单例合约容器，存储所有合约代码,无需加锁，因为只有初始化是单线程的，之后只读访问，线程安全
class ContractContainer {
public:
    using ContractList = std::vector<std::string>;

    static ContractContainer& instance() {
        static ContractContainer inst;
        return inst;
    }

    ContractContainer(const ContractContainer&) = delete;
    ContractContainer& operator=(const ContractContainer&) = delete;

    void addContract(const std::string& contract) {
        contracts_.push_back(contract);
    }

    void addContracts(const std::vector<std::string>& contracts) {
        contracts_.insert(contracts_.end(), contracts.begin(), contracts.end());
    }

    const ContractList& getAllContracts() const {
        return contracts_;  // 初始化完成后只读，安全
    }

    void clear() {
        contracts_.clear();
    }

    size_t size() const {
        return contracts_.size();
    }

    bool empty() const {
        return contracts_.empty();
    }

private:
    ContractContainer() = default;  
    ContractList contracts_;
};

/*使用方法
ContractContainer::instance().addContracts({ "rb2401", "rb2405", "hc2401" });

// 在 Dispatcher 初始化时获取合约列表
const auto& contracts = ContractContainer::instance().getAllContracts();
Dispatcher::initDispatcher(contracts, tickQueues, orderQueues, tradeQueues);
*/