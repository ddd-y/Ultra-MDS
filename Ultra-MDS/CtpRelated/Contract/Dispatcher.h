#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <string_view>
#include "TickData.h"
#include "SPSCQueue.h"
#include"../DataHandler/UnitProcessor.h"

//存储一个unitprocessor的索引和contract的索引
struct DispatchUnit 
{
    int Index_Contract;
    int Index_UnitProcessor;
};
// 透明哈希，允许直接用 std::string_view 查找
struct TransparentHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

class Dispatcher {
private:

    //这俩生命周期还是由systemstarter管理
    std::vector<rigtorp::SPSCQueue<TickData>*> contracts_;
    std::vector<UnitProcessor*> UnitProcessors;

    std::unordered_map<std::string, DispatchUnit, TransparentHash, std::equal_to<>> mapping_;
    static Dispatcher instance;
    
public:
    static void initDispatcher(
        const std::vector<std::string>& contractList,
        const std::vector<rigtorp::SPSCQueue<TickData>*>& tickQueues,
        const std::vector<UnitProcessor*> n_processors) {

        instance.UnitProcessors = n_processors;

        const size_t n = tickQueues.size();
        instance.contracts_.reserve(n);
        instance.contracts_ = tickQueues;

        // 轮询分配合约到线程
        const size_t contract_size  = n_processors.size();
        int threadIdx_ = 0;
        for (int i = 0; i < contract_size; ++i) 
        {
            instance.mapping_.emplace(n_processors[i]->getContractName(), DispatchUnit{threadIdx_,i});
            threadIdx_= (threadIdx_ + 1) % n;
        }
    }

	// 这个函数会被行情线程调用，分发tickdata到对应的队列
    static void dispatch(const CThostFtdcDepthMarketDataField& data) {
        auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto it = instance.mapping_.find(std::string_view(data.InstrumentID));
        if (it != instance.mapping_.end()) {
            if (!instance.contracts_[it->second.Index_Contract]->try_emplace
            (data, instance.UnitProcessors[it->second.Index_UnitProcessor],now))
            {
                LOG_WARN("有行情数据被丢弃");
            }
        }
    }
};