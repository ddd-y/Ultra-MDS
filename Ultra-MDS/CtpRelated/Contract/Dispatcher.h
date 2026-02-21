#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <string_view>
#include "TickData.h"
#include "SPSCQueue.h"
#include"../DataHandler/detector.h"

//存储一个detector的索引和contract的索引
struct DispatchUnit 
{
    int Index_Contract;
    int Index_Detector;
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
    std::vector<Detector*> Detectors;

    std::unordered_map<std::string, DispatchUnit, TransparentHash, std::equal_to<>> mapping_;
    static Dispatcher instance;
    
    //用来计数，因为队列满了而丢掉的tickdata有多少
    int drop_count_tick = 0;
public:
    static void initDispatcher(
        const std::vector<std::string>& contractList,
        const std::vector<rigtorp::SPSCQueue<TickData>*>& tickQueues,
        const std::vector<Detector*> n_detectors) {

        instance.Detectors = n_detectors;

        const size_t n = tickQueues.size();
        instance.contracts_.reserve(n);
        instance.contracts_ = tickQueues;

        // 轮询分配合约到线程
        const size_t contract_size  = contractList.size();
        int threadIdx_ = 0;
        for (int i = 0; i < contract_size; ++i) 
        {
            instance.mapping_.emplace(contractList[i], DispatchUnit{ threadIdx_,i });
            threadIdx_= (threadIdx_ + 1) % n;
        }
    }

	// 这个函数会被行情线程调用，分发tickdata到对应的队列
    static void dispatch(const CThostFtdcDepthMarketDataField& data) {
        auto it = instance.mapping_.find(std::string_view(data.InstrumentID));
        if (it != instance.mapping_.end()) {
            if(instance.contracts_[it->second.Index_Contract]->try_emplace
            (data, instance.Detectors[it->second.Index_Detector]))
                ++instance.drop_count_tick;
        }
    }
};