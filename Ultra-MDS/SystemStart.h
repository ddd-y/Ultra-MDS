#pragma once
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include "../CtpRelated/MarketSPI.h"
#include "../CtpRelated/Contract/Dispatcher.h"
#include "SPSCQueue.h"
#include "../DataHandler/TickHandler.h"
#include"CtpRelated/Contract/Container.h"
#include"../configrelated/ConfigRead.h"
#include"../configrelated/Loginer.h"
#include "../DataHandler/UnitProcessor.h"

constexpr const char* CONFIG_PATH = "./myconfig.json";
constexpr const int MAX_TICK_QUEUE_SIZE = 8;

inline int getBizCoreId(int thread_idx, long total_cpu_count) {
    if (total_cpu_count <= 1) return 0;
    int base_core = thread_idx;
    return base_core % total_cpu_count;
}

class SystemStarter {
private:
    std::vector<std::unique_ptr<TickHandler>> tickHandlers_;
    std::vector<std::unique_ptr<UnitProcessor>> UnitProcessors_;
    ConfigReader m_configReader; // 全局配置读取实例
    json m_globalValidationRules; // 缓存全局校验规则

    // 配置加载合约列表到ContractContainer
    void loadContractList() {
        std::vector<std::string> contracts = m_configReader.getStringArray({ "subscribed_contracts" });
        if (contracts.empty()) {
            LOG_ERROR("配置文件中未找到订阅合约列表，系统无法正常运行");
            throw std::runtime_error("订阅合约列表为空");
        }
        ContractContainer::instance().clear();
        ContractContainer::instance().addContracts(contracts);
        LOG_INFO("从配置加载订阅合约成功，共{}个合约", contracts.size());
    }

    void HandlerAndDispatcherStart()
    {
        const std::vector<std::string>& contractList = ContractContainer::instance().getAllContracts();
        int contract_num = static_cast<int>(contractList.size());

        // 改造：为每个合约创建带配置的UnitProcessor实例
        std::vector<UnitProcessor*> tempUnitProcessors;
        tempUnitProcessors.reserve(contract_num);
        for (const auto& contract : contractList)
        {
            // 读取该合约的专属配置
            auto contractConfig = m_configReader.getContractValidationConfig(contract);
            // 创建UnitProcessor实例
            UnitProcessors_.emplace_back(std::make_unique<UnitProcessor>(contract, m_globalValidationRules, contractConfig));
            tempUnitProcessors.push_back(UnitProcessors_.back().get());
        }

        // CPU核心分配、TickHandler创建
        long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
        if (cpu_count <= 0) {
            LOG_ERROR("获取CPU核心数失败，使用默认值1");
            cpu_count = 1;
        }
        if (cpu_count == 1) {
            LOG_WARN("系统仅1个CPU核心，无法避开系统核心，业务线程将与系统共享CPU0");
        }
        int available_core_num = (cpu_count > 1) ? (cpu_count) : 1;
        int thread_num = std::max(1, std::min(available_core_num, contract_num));
        LOG_INFO("检测到CPU核心数：{}，可用业务核心数：{}，合约数量：{}，最终初始化TickHandler数：{}",
            cpu_count, available_core_num, contract_num, thread_num);

        std::vector<rigtorp::SPSCQueue<TickData>*> tick_queues;
        tick_queues.reserve(thread_num);
        tickHandlers_.reserve(thread_num);

        int multisize = contract_num / thread_num;
        multisize = std::max(1, multisize);
        LOG_INFO("单个线程处理合约倍数(multisize)为：{}", multisize);
        int queue_size = multisize * MAX_TICK_QUEUE_SIZE;

        for (int i = 0; i < thread_num; ++i) {
            auto* tickQ = new rigtorp::SPSCQueue<TickData>(queue_size);
            tick_queues.push_back(tickQ);
            int core_id = getBizCoreId(i, cpu_count);
            auto handler = std::make_unique<TickHandler>(tickQ);
            handler->start(core_id);
            tickHandlers_.emplace_back(std::move(handler));
            LOG_INFO("TickHandler[{}] 启动并绑定核心：Core {}", i, core_id);
        }

        // Dispatcher初始化
        Dispatcher::initDispatcher(contractList, tick_queues, tempUnitProcessors);
    }

    void ConfigInit()
    {
        try {
            // 读取登录信息
            std::string brokerID = m_configReader.getString(std::vector<std::string>{ "login", "brokerID" });
            std::string userID = m_configReader.getString(std::vector<std::string>{ "login", "userID" });
            std::string password = m_configReader.getString(std::vector<std::string>{ "login", "password" });
            std::string tcpAddress = m_configReader.getString(std::vector<std::string>{ "login", "tcpAddress" });

            // 设置到Loginer单例
            Loginer& loginer = Loginer::getInstance();
            loginer.setBrokerId(brokerID);
            loginer.setUserId(userID); 
            loginer.setPassword(password);
            loginer.setTcpAddress(tcpAddress);

            // 缓存全局校验规则
            m_globalValidationRules = m_configReader.getGlobalValidationConfig();

            // 加载合约列表
            loadContractList();

            LOG_INFO("配置文件读取成功，登录参数、合约列表、校验规则已加载");
        }
        catch (const std::exception& e) {
            LOG_ERROR("配置初始化失败：{}", e.what());
            throw; // 配置失败直接终止启动，避免脏数据运行
        }
    }



public:
    SystemStarter() : m_configReader(CONFIG_PATH) {
        Ultra::Logger::getInstance().init(DEFAULT_LOG_PATH, spdlog::level::trace);
        ConfigInit();
        HandlerAndDispatcherStart();
        MarketManager::Init();
    }

    SystemStarter(const SystemStarter&) = delete;
    SystemStarter& operator=(const SystemStarter&) = delete;
    ~SystemStarter() = default;
};

