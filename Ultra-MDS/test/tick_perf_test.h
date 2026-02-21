#pragma once
#include "TickData.h"
#include "tick_data.pb.h"  // 替换为实际生成的proto头文件路径
#include <ThostFtdcMdApi.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// 初始化Google Protobuf
void initProtobuf() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

// 生成模拟的CTP深度行情数据（填充测试用的有效值）
CThostFtdcDepthMarketDataField generateTestCTPData() {
    CThostFtdcDepthMarketDataField ctpData{};  // 初始化为0

    // 填充基础字符串字段
    strncpy(ctpData.InstrumentID, "rb2405", sizeof(ctpData.InstrumentID) - 1);
    strncpy(ctpData.ExchangeID, "SHFE", sizeof(ctpData.ExchangeID) - 1);
    strncpy(ctpData.UpdateTime, "14:30:00", sizeof(ctpData.UpdateTime) - 1);
    strncpy(ctpData.ActionDay, "20240520", sizeof(ctpData.ActionDay) - 1);
    strncpy(ctpData.TradingDay, "20240520", sizeof(ctpData.TradingDay) - 1);

    // 填充价格字段
    ctpData.LastPrice = 4500.0;
    ctpData.UpperLimitPrice = 4800.0;
    ctpData.LowerLimitPrice = 4200.0;
    ctpData.BidPrice1 = 4499.0;
    ctpData.BidPrice2 = 4498.0;
    ctpData.BidPrice3 = 4497.0;
    ctpData.BidPrice4 = 4496.0;
    ctpData.BidPrice5 = 4495.0;
    ctpData.AskPrice1 = 4501.0;
    ctpData.AskPrice2 = 4502.0;
    ctpData.AskPrice3 = 4503.0;
    ctpData.AskPrice4 = 4504.0;
    ctpData.AskPrice5 = 4505.0;

    // 填充成交量字段
    ctpData.Volume = 1000;
    ctpData.UpdateMillisec = 500;
    ctpData.BidVolume1 = 100;
    ctpData.BidVolume2 = 200;
    ctpData.BidVolume3 = 300;
    ctpData.BidVolume4 = 400;
    ctpData.BidVolume5 = 500;
    ctpData.AskVolume1 = 150;
    ctpData.AskVolume2 = 250;
    ctpData.AskVolume3 = 350;
    ctpData.AskVolume4 = 450;
    ctpData.AskVolume5 = 550;

    return ctpData;
}

// 测试自定义TickData初始化耗时
// @param loopCount: 循环次数（建议1e6+，减少计时误差）
// @param ctpData: 测试用的CTP数据
double testCustomTickDataInit(size_t loopCount, const CThostFtdcDepthMarketDataField& ctpData) {
    using namespace std::chrono;

    // 预热（避免首次执行的缓存/初始化开销）
    for (size_t i = 0; i < 10000; ++i) {
        TickData tick(ctpData, nullptr);
    }

    // 开始计时
    auto start = high_resolution_clock::now();

    // 核心测试逻辑：循环初始化自定义TickData
    // 用vector存储对象，避免编译器优化掉空操作
    std::vector<TickData> tickList;
    tickList.reserve(loopCount);  // 预分配内存，避免扩容耗时
    for (size_t i = 0; i < loopCount; ++i) {
        tickList.emplace_back(ctpData, nullptr);
    }

    // 结束计时
    auto end = high_resolution_clock::now();
    duration<double, std::milli> totalMs = end - start;

    // 返回单次初始化耗时（微秒）
    return (totalMs.count() * 1000) / loopCount;
}

// 测试Protobuf TickDataMes初始化耗时
// @param loopCount: 循环次数
// @param ctpData: 测试用的CTP数据
double testProtoTickDataInit(size_t loopCount, const CThostFtdcDepthMarketDataField& ctpData) {
    using namespace std::chrono;

    // 预热
    TickDataMes protoTick;
    for (size_t i = 0; i < 10000; ++i) {
        protoTick.Clear();  // 清空之前的数据
        // 模拟赋值逻辑
        protoTick.set_last_price(ctpData.LastPrice);
        protoTick.set_limit_up(ctpData.UpperLimitPrice);
        protoTick.set_limit_down(ctpData.LowerLimitPrice);
        protoTick.set_volume(ctpData.Volume);
        protoTick.set_updatemill(ctpData.UpdateMillisec);
        protoTick.set_local_receive_time(0);
        protoTick.set_instrumentid(ctpData.InstrumentID);
        protoTick.set_exchangeid(ctpData.ExchangeID);
        protoTick.set_update_time(ctpData.UpdateTime);
        protoTick.set_action_day(ctpData.ActionDay);
        protoTick.set_tradingday(ctpData.TradingDay);

        // 填充五档bid/ask价格和成交量
        protoTick.clear_bid_price();
        protoTick.add_bid_price(ctpData.BidPrice1);
        protoTick.add_bid_price(ctpData.BidPrice2);
        protoTick.add_bid_price(ctpData.BidPrice3);
        protoTick.add_bid_price(ctpData.BidPrice4);
        protoTick.add_bid_price(ctpData.BidPrice5);

        protoTick.clear_ask_price();
        protoTick.add_ask_price(ctpData.AskPrice1);
        protoTick.add_ask_price(ctpData.AskPrice2);
        protoTick.add_ask_price(ctpData.AskPrice3);
        protoTick.add_ask_price(ctpData.AskPrice4);
        protoTick.add_ask_price(ctpData.AskPrice5);

        protoTick.clear_bid_volume();
        protoTick.add_bid_volume(ctpData.BidVolume1);
        protoTick.add_bid_volume(ctpData.BidVolume2);
        protoTick.add_bid_volume(ctpData.BidVolume3);
        protoTick.add_bid_volume(ctpData.BidVolume4);
        protoTick.add_bid_volume(ctpData.BidVolume5);

        protoTick.clear_ask_volume();
        protoTick.add_ask_volume(ctpData.AskVolume1);
        protoTick.add_ask_volume(ctpData.AskVolume2);
        protoTick.add_ask_volume(ctpData.AskVolume3);
        protoTick.add_ask_volume(ctpData.AskVolume4);
        protoTick.add_ask_volume(ctpData.AskVolume5);
    }

    // 开始计时
    auto start = high_resolution_clock::now();

    // 核心测试逻辑：循环初始化Protobuf对象
    std::vector<TickDataMes> protoList;
    protoList.reserve(loopCount);
    for (size_t i = 0; i < loopCount; ++i) {
        TickDataMes tick;
        tick.set_last_price(ctpData.LastPrice);
        tick.set_limit_up(ctpData.UpperLimitPrice);
        tick.set_limit_down(ctpData.LowerLimitPrice);
        tick.set_volume(ctpData.Volume);
        tick.set_updatemill(ctpData.UpdateMillisec);
        tick.set_local_receive_time(0);  // 测试中忽略local_receive_time的实时值
        tick.set_instrumentid(ctpData.InstrumentID);
        tick.set_exchangeid(ctpData.ExchangeID);
        tick.set_update_time(ctpData.UpdateTime);
        tick.set_action_day(ctpData.ActionDay);
        tick.set_tradingday(ctpData.TradingDay);

        // 填充五档行情
        tick.add_bid_price(ctpData.BidPrice1);
        tick.add_bid_price(ctpData.BidPrice2);
        tick.add_bid_price(ctpData.BidPrice3);
        tick.add_bid_price(ctpData.BidPrice4);
        tick.add_bid_price(ctpData.BidPrice5);

        tick.add_ask_price(ctpData.AskPrice1);
        tick.add_ask_price(ctpData.AskPrice2);
        tick.add_ask_price(ctpData.AskPrice3);
        tick.add_ask_price(ctpData.AskPrice4);
        tick.add_ask_price(ctpData.AskPrice5);

        tick.add_bid_volume(ctpData.BidVolume1);
        tick.add_bid_volume(ctpData.BidVolume2);
        tick.add_bid_volume(ctpData.BidVolume3);
        tick.add_bid_volume(ctpData.BidVolume4);
        tick.add_bid_volume(ctpData.BidVolume5);

        tick.add_ask_volume(ctpData.AskVolume1);
        tick.add_ask_volume(ctpData.AskVolume2);
        tick.add_ask_volume(ctpData.AskVolume3);
        tick.add_ask_volume(ctpData.AskVolume4);
        tick.add_ask_volume(ctpData.AskVolume5);

        protoList.push_back(tick);
    }

    // 结束计时
    auto end = high_resolution_clock::now();
    duration<double, std::milli> totalMs = end - start;

    // 返回单次初始化耗时（微秒）
    return (totalMs.count() * 1000) / loopCount;
}

void tick_test() {
    // 初始化Protobuf
    initProtobuf();

    // 1. 生成测试用的CTP数据
    CThostFtdcDepthMarketDataField testCTPData = generateTestCTPData();

    // 2. 设置测试参数（循环次数建议根据机器性能调整，1e6 ~ 1e7）
    const size_t LOOP_COUNT = 10000000;  // 1000万次循环
    std::cout << "===== TickData 初始化性能测试 =====" << std::endl;
    std::cout << "测试循环次数: " << LOOP_COUNT << " 次" << std::endl;
    std::cout << std::fixed << std::setprecision(6);  // 保留6位小数

    // 3. 测试自定义TickData
    double customTickTimeUs = testCustomTickDataInit(LOOP_COUNT, testCTPData);
    std::cout << "自定义TickData 单次初始化耗时: " << customTickTimeUs << " 微秒" << std::endl;

    // 4. 测试Protobuf TickDataMes
    double protoTickTimeUs = testProtoTickDataInit(LOOP_COUNT, testCTPData);
    std::cout << "Protobuf TickDataMes 单次初始化耗时: " << protoTickTimeUs << " 微秒" << std::endl;

    // 5. 对比结果
    double ratio = protoTickTimeUs / customTickTimeUs;
    std::cout << "Protobuf耗时是自定义TickData的: " << ratio << " 倍" << std::endl;

    // 清理Protobuf
    google::protobuf::ShutdownProtobufLibrary();
}