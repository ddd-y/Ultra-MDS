#pragma once
#include"ThostFtdcMdApi.h"
#include<chrono>
#include<cstring>
#include"tick_data.pb.h"

//防止重复定义
class Detector;
struct TickData {
    double last_price = 0;     // 最新价
    double bid_price[5] = { 0 };// 五档行情
    double ask_price[5] = { 0 };
    double limit_up = 0;       // 涨停
    double limit_down = 0;     // 跌停

    int volume = 0;         // 成交量
    int updatemill = 0;//更新毫秒，对应CTP数据的UpdateMillisec
    int bid_volume[5] = { 0 };
    int ask_volume[5] = { 0 };


    Detector* m_detector = nullptr;
    int64_t local_receive_time = 0;
    char InstrumentID[81];
    char ExchangeID[9];  
    char update_time[9];
    char Action_Day[9];
    char TradingDay[9];

    // 构造函数中拷贝ExchangeID
    TickData(const CThostFtdcDepthMarketDataField& Data, Detector* detector)
        :last_price(Data.LastPrice),
        limit_up(Data.UpperLimitPrice),
        limit_down(Data.LowerLimitPrice),
        bid_price{ Data.BidPrice1,Data.BidPrice2,Data.BidPrice3,Data.BidPrice4,Data.BidPrice5 },
        ask_price{ Data.AskPrice1,Data.AskPrice2,Data.AskPrice3,Data.AskPrice4,Data.AskPrice5 },
        volume(Data.Volume),
        updatemill(Data.UpdateMillisec),
        m_detector(detector)
    {
        local_receive_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
        std::memcpy(InstrumentID, Data.InstrumentID, sizeof(InstrumentID));
        std::memcpy(ExchangeID, Data.ExchangeID, sizeof(ExchangeID)); 
        std::memcpy(update_time, Data.UpdateTime, sizeof(update_time));
        std::memcpy(Action_Day, Data.ActionDay, sizeof(Action_Day));
		std::memcpy(TradingDay, Data.TradingDay, sizeof(TradingDay));
    }
    TickData() = default;
};