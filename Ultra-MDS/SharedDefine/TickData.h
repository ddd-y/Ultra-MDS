#pragma once
#include"ThostFtdcMdApi.h"
#include<chrono>
#include<cstring>
#include"tick_data.pb.h"

//防止重复定义
class UnitProcessor;
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


    UnitProcessor* m_detector = nullptr;

	// 本地接收时间戳（纳秒级），用于计算延迟
    int64_t local_receive_time = 0;
    char InstrumentID[81];
    char ExchangeID[9];  
    char update_time[9];
    char Action_Day[9];
    char TradingDay[9];

    // 构造函数中拷贝ExchangeID
    TickData(const CThostFtdcDepthMarketDataField& Data, UnitProcessor* detector)
        :last_price(Data.LastPrice),
        limit_up(Data.UpperLimitPrice),
        limit_down(Data.LowerLimitPrice),
        bid_price{ Data.BidPrice1,Data.BidPrice2,Data.BidPrice3,Data.BidPrice4,Data.BidPrice5 },
        ask_price{ Data.AskPrice1,Data.AskPrice2,Data.AskPrice3,Data.AskPrice4,Data.AskPrice5 },
        volume(Data.Volume),
        updatemill(Data.UpdateMillisec),
        m_detector(detector),
        local_receive_time(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count())
    {
        std::memcpy(InstrumentID, Data.InstrumentID, sizeof(InstrumentID));
        std::memcpy(ExchangeID, Data.ExchangeID, sizeof(ExchangeID)); 
        std::memcpy(update_time, Data.UpdateTime, sizeof(update_time));
        std::memcpy(Action_Day, Data.ActionDay, sizeof(Action_Day));
		std::memcpy(TradingDay, Data.TradingDay, sizeof(TradingDay));
    }

    inline void fillProtoData(TickDataMes& mes) const {
        // 基础价格字段
        mes.set_last_price(this->last_price);
        mes.set_limit_up(this->limit_up);
        mes.set_limit_down(this->limit_down);

        // 五档买价（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_bid_price(this->bid_price[i]);
        }

        // 五档卖价（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_ask_price(this->ask_price[i]);
        }

        // 成交量、更新毫秒
        mes.set_volume(this->volume);
        mes.set_updatemill(this->updatemill);

        // 五档买量（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_bid_volume(this->bid_volume[i]);
        }

        // 五档卖量（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_ask_volume(this->ask_volume[i]);
        }

        // 字符串字段（安全处理长度，避免越界）
        mes.set_instrumentid(std::string(this->InstrumentID, strnlen(this->InstrumentID, sizeof(this->InstrumentID))));
        mes.set_exchangeid(std::string(this->ExchangeID, strnlen(this->ExchangeID, sizeof(this->ExchangeID))));
        mes.set_update_time(std::string(this->update_time, strnlen(this->update_time, sizeof(this->update_time))));
        mes.set_action_day(std::string(this->Action_Day, strnlen(this->Action_Day, sizeof(this->Action_Day))));
        mes.set_tradingday(std::string(this->TradingDay, strnlen(this->TradingDay, sizeof(this->TradingDay))));
    }

    TickData() = default;
};