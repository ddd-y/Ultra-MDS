#include <benchmark/benchmark.h>
#include <random>
#include "UnitProcessor.h"
#include "TickData.h"
#include "Logger.h"

// ===================== 辅助函数：构造测试用TickData和UnitProcessor =====================
// 初始化空Logger（避免日志干扰基准测试）
void initTestLogger() {
    // 若Logger有初始化接口，此处调用；若无则忽略（确保日志不输出）
    static bool first = false;
    if (!first)
    {
        Ultra::Logger::getInstance().init(DEFAULT_LOG_PATH, spdlog::level::trace);
		first = true;
    }
}

// 构造默认测试用TickData
TickData createTestTickData() {
    TickData tick;
    // 基础字段初始化
    tick.last_price = 100.0;
    tick.limit_up = 110.0;
    tick.limit_down = 90.0;
    // 五档买卖价（正常逻辑：买1<买2<...，卖1>卖2>...）
    tick.bid_price[0] = 99.5; tick.bid_price[1] = 99.0; tick.bid_price[2] = 98.5; tick.bid_price[3] = 98.0; tick.bid_price[4] = 97.5;
    tick.ask_price[0] = 100.5; tick.ask_price[1] = 101.0; tick.ask_price[2] = 101.5; tick.ask_price[3] = 102.0; tick.ask_price[4] = 102.5;
    // 成交量
    tick.volume = 1000;
    tick.updatemill = 500; // 毫秒
    tick.bid_volume[0] = 100; tick.bid_volume[1] = 90; tick.bid_volume[2] = 80; tick.bid_volume[3] = 70; tick.bid_volume[4] = 60;
    tick.ask_volume[0] = 100; tick.ask_volume[1] = 90; tick.ask_volume[2] = 80; tick.ask_volume[3] = 70; tick.ask_volume[4] = 60;
    // 时间/交易所/合约字段
    std::strcpy(tick.update_time, "10:00:00");
    std::strcpy(tick.TradingDay, "20240520");
    std::strcpy(tick.Action_Day, "20240520");
    std::strcpy(tick.ExchangeID, "SHFE");
    std::strcpy(tick.InstrumentID, "rb2410");
    return tick;
}

// 构造测试用UnitProcessor（全局配置+合约配置为空）
UnitProcessor createTestUnitProcessor() {
    json globalJson;
    std::optional<json> contractJson = std::nullopt;
    // 初始化全局配置（白名单添加SHFE）
    globalJson["exchange_whitelist"] = json::array({ "SHFE", "CZCE" });
    globalJson["timestamp_base"] = "TradingDay";
    globalJson["max_disorder_tolerance_ms"] = 10;
    globalJson["allow_same_ms_data"] = true;
    globalJson["check_bid_ask_levels"] = 5;
    globalJson["max_invert_spread_tick"] = 1;
    globalJson["tick_size"] = 0.5;
    return UnitProcessor("rb2410", globalJson, contractJson);
}

// ===================== 基准测试用例 =====================
// 1. 测试CheckWhiteList性能
static void BM_CheckWhiteList(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();

    // 基准测试循环（state.iterations()为框架自动控制的迭代次数）
    for (auto _ : state) {
        // 注意：需临时将CheckWhiteList改为public才能调用
        benchmark::DoNotOptimize(processor.CheckWhiteList(tick));
    }
}
BENCHMARK(BM_CheckWhiteList);

// 2. 测试checkTimestampNew性能
static void BM_checkTimestampNew(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();

    for (auto _ : state) {
        // 注意：需临时将checkTimestampNew改为public才能调用
        benchmark::DoNotOptimize(processor.checkTimestampNew(tick));
    }
}
BENCHMARK(BM_checkTimestampNew);

// 3. 测试checkPriceValidity性能
static void BM_checkPriceValidity(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();

    for (auto _ : state) {
        // 注意：需临时将checkPriceValidity改为public才能调用
        benchmark::DoNotOptimize(processor.checkPriceValidity(tick));
    }
}
BENCHMARK(BM_checkPriceValidity);

// 4. 测试checkVolumeValidity性能
static void BM_checkVolumeValidity(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();

    for (auto _ : state) {
        // 注意：需临时将checkVolumeValidity改为public才能调用
        benchmark::DoNotOptimize(processor.checkVolumeValidity(tick));
    }
}
BENCHMARK(BM_checkVolumeValidity);

// 5. 测试checkAbnormalPacket性能
static void BM_checkAbnormalPacket(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();

    for (auto _ : state) {
        // 注意：需临时将checkAbnormalPacket改为public才能调用
        benchmark::DoNotOptimize(processor.checkAbnormalPacket(tick));
    }
}
BENCHMARK(BM_checkAbnormalPacket);

// 6. 测试「全量校验」性能（模拟真实场景）
static void BM_validate_AllChecks(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();

    for (auto _ : state) {
        benchmark::DoNotOptimize(processor.validate(tick));
    }
}
BENCHMARK(BM_validate_AllChecks);

// ===================== 边界场景测试（可选） =====================
// 测试checkPriceValidity（边界值：涨跌停临界、档位价格相等）
static void BM_checkPriceValidity_Boundary(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();
    // 构造边界场景：最新价=涨停价（带容忍度）、档位价格相等
    tick.last_price = tick.limit_up * (1 + 0.0001); // 刚好在容忍范围内
    tick.bid_price[1] = tick.bid_price[0]; // 买2=买1（合法）
    tick.ask_price[1] = tick.ask_price[0]; // 卖2=卖1（合法）

    for (auto _ : state) {
        benchmark::DoNotOptimize(processor.checkPriceValidity(tick));
    }
}
BENCHMARK(BM_checkPriceValidity_Boundary);

// 测试checkTimestampNew（乱序场景）
static void BM_checkTimestampNew_Disorder(benchmark::State& state) {
    initTestLogger();
    auto processor = createTestUnitProcessor();
    auto tick = createTestTickData();
    // 先调用一次初始化lastTimestamp
    processor.checkTimestampNew(tick);
    // 构造乱序时间戳（比上次小5ms，在容忍范围内）
    tick.updatemill = 495;

    for (auto _ : state) {
        benchmark::DoNotOptimize(processor.checkTimestampNew(tick));
    }
}
BENCHMARK(BM_checkTimestampNew_Disorder);

