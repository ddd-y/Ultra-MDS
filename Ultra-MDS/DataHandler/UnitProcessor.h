#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <optional>
#include "TickData.h"
#include "Logger.h"
#include "../Tool/nlohmann/json.hpp"
#include <cfloat>
#include <boost/asio.hpp>

using json = nlohmann::json;
namespace asio = boost::asio;

constexpr const int MAX_BID_ASK_LEVELS = 5; 

struct UDPConfig
{
    std::string Multi_add = "";
    int port = 8888;
    asio::ip::udp::endpoint target_endpoint; // 目标UDP端点（IP + 端口）
};
//针对单个合约的特色配置
// ====================
struct SpecialConfig
{
	UDPConfig m_udpConfig; // UDP组播配置

   // 校验总开关
    bool enable_exchange_check = true;
    bool enable_timestamp_check = true;
    bool enable_price_check = true;
    bool enable_volume_check = true;
    bool enable_abnormal_packet_check = true;

    // 时间戳配置
    std::string timestamp_base = "TradingDay";
    int max_disorder_tolerance_ms = 10;
    bool allow_same_ms_data = true;

    // 价格/买卖盘配置
    int check_bid_ask_levels = 1;
    int max_invert_spread_tick = 1;
    double tick_size = 1.0;
    double limit_price_tolerance_ratio = 0.0001;
    bool skip_zero_limit_price = true;

    //白名单
	std::vector<std::string> exchange_whitelist;

    // 从JSON加载配置（合约专属规则覆盖全局默认）
    static SpecialConfig fromJson(const json& globalJson, const std::optional<json>& contractJson) {
        SpecialConfig config;

        // 先加载全局默认规则
        if (!globalJson.is_null()) {
			config.m_udpConfig.port = globalJson.value("port", config.m_udpConfig.port);
			config.m_udpConfig.Multi_add = globalJson.value("Multi_add", config.m_udpConfig.Multi_add);
            config.enable_exchange_check = globalJson.value("enable_exchange_check", config.enable_exchange_check);
            config.enable_timestamp_check = globalJson.value("enable_timestamp_check", config.enable_timestamp_check);
            config.enable_price_check = globalJson.value("enable_price_check", config.enable_price_check);
            config.enable_volume_check = globalJson.value("enable_volume_check", config.enable_volume_check);
            config.enable_abnormal_packet_check = globalJson.value("enable_abnormal_packet_check", config.enable_abnormal_packet_check);

            config.timestamp_base = globalJson.value("timestamp_base", config.timestamp_base);
            config.max_disorder_tolerance_ms = globalJson.value("max_disorder_tolerance_ms", config.max_disorder_tolerance_ms);
            config.allow_same_ms_data = globalJson.value("allow_same_ms_data", config.allow_same_ms_data);

            config.check_bid_ask_levels = std::clamp(globalJson.value("check_bid_ask_levels", config.check_bid_ask_levels), 1, 5);
            config.max_invert_spread_tick = globalJson.value("max_invert_spread_tick", config.max_invert_spread_tick);
            config.tick_size = globalJson.value("tick_size", config.tick_size);
            if (config.tick_size <= 0) {
                LOG_INFO("tick_size设置有误，小于0，设回默认值1.0");
                config.tick_size = 1.0;
            }
            config.limit_price_tolerance_ratio = globalJson.value("limit_price_tolerance_ratio", config.limit_price_tolerance_ratio);
            config.skip_zero_limit_price = globalJson.value("skip_zero_limit_price", config.skip_zero_limit_price);
			// 加载全局白名单
            if (globalJson.contains("exchange_whitelist") && globalJson["exchange_whitelist"].is_array()) {
                for (const auto& exch : globalJson["exchange_whitelist"]) {
                    config.exchange_whitelist.push_back(exch.get<std::string>());
                }
            }
        }

        // 再用合约专属规则覆盖
        if (contractJson.has_value() && !contractJson.value().is_null()) {
            const json& cJson = contractJson.value();
			if (cJson.contains("port")) config.m_udpConfig.port = cJson["port"];
			if (cJson.contains("Multi_add")) config.m_udpConfig.Multi_add = cJson["Multi_add"];
            if (cJson.contains("enable_exchange_check")) config.enable_exchange_check = cJson["enable_exchange_check"];
            if (cJson.contains("enable_timestamp_check")) config.enable_timestamp_check = cJson["enable_timestamp_check"];
            if (cJson.contains("enable_price_check")) config.enable_price_check = cJson["enable_price_check"];
            if (cJson.contains("enable_volume_check")) config.enable_volume_check = cJson["enable_volume_check"];
            if (cJson.contains("enable_abnormal_packet_check")) config.enable_abnormal_packet_check = cJson["enable_abnormal_packet_check"];

            if (cJson.contains("timestamp_base")) config.timestamp_base = cJson["timestamp_base"];
            if (cJson.contains("max_disorder_tolerance_ms")) config.max_disorder_tolerance_ms = cJson["max_disorder_tolerance_ms"];
            if (cJson.contains("allow_same_ms_data")) config.allow_same_ms_data = cJson["allow_same_ms_data"];

            if (cJson.contains("check_bid_ask_levels"))
                config.check_bid_ask_levels = std::clamp(cJson["check_bid_ask_levels"].get<int>(), 1, 5);
            if (cJson.contains("max_invert_spread_tick")) config.max_invert_spread_tick = cJson["max_invert_spread_tick"];
			if (cJson.contains("tick_size")) config.tick_size = cJson["tick_size"];
            if (config.tick_size <= 0) {
                LOG_INFO("tick_size设置有误，小于0，设回默认值1.0");
                config.tick_size = 1.0;
            }
            if (cJson.contains("limit_price_tolerance_ratio")) config.limit_price_tolerance_ratio = cJson["limit_price_tolerance_ratio"];
            if (cJson.contains("skip_zero_limit_price")) config.skip_zero_limit_price = cJson["skip_zero_limit_price"];
            
            if (cJson.contains("exchange_whitelist") && cJson["exchange_whitelist"].is_array()) {
                config.exchange_whitelist.clear();
                for (const auto& exch : cJson["exchange_whitelist"]) {
                    config.exchange_whitelist.push_back(exch.get<std::string>());
                }
            }
        }

        try {
            config.m_udpConfig.target_endpoint = asio::ip::udp::endpoint(asio::ip::make_address(config.m_udpConfig.Multi_add), config.m_udpConfig.port);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Multi_add({}) 不是合法IP地址，异常：{}", config.m_udpConfig.Multi_add, e.what());
            // 重置为默认值或标记无效
            config.m_udpConfig.Multi_add = "";
            config.m_udpConfig.port = 8888;
        }
        return config;
    }
};

// 负责data的检验以及存储UDP组播发送的目标地址
class UnitProcessor
{
private:
    int64_t m_lastTimestamp = 0;
    std::string m_contractName; // 绑定的合约名称
    SpecialConfig m_config;   // 该合约专属的校验配置


    inline bool strToInt64 (const char* str, size_t len, int64_t& out) const{
        out = 0;
        for (size_t i = 0; i < len; ++i) {
            if (!isdigit(str[i])) return false;
            out = out * 10 + (str[i] - '0');
        }
        return true;
    }

    inline bool isInvalidPrice(double price) const
    {
        return std::isnan(price) || std::isinf(price) || price < 0 || price == DBL_MAX;
    }

    inline bool CheckWhiteList(const TickData& tick) const
    {
        const size_t exchLen = strnlen(tick.ExchangeID, sizeof(tick.ExchangeID));
        const std::string exchId(tick.ExchangeID, exchLen);
        if (!m_config.exchange_whitelist.empty()) {
            // 线性查找：std::find返回end()表示未找到
            auto it = std::find(m_config.exchange_whitelist.begin(), m_config.exchange_whitelist.end(), exchId);
            if (it == m_config.exchange_whitelist.end()) 
            {
                LOG_INFO("[Tick基础校验失败] 交易所代码不在白名单 | 合约: {} | 交易所: {} ",tick.InstrumentID, exchId);
                return false;
            }
        }
		//若白名单为空，则默认放行所有交易所
        return true;
    }

    // 配置化：时间戳基准切换（ActionDay/TradingDay）
    inline int64_t combineTimestamp(const TickData& tick) const
    {
        // 校验冒号位置
        if (tick.update_time[2] != ':' || tick.update_time[5] != ':')
        {
            LOG_INFO("[Tick校验失败] 时间格式错误（冒号位置不对） | 合约: {} | 时间: {}",
                tick.InstrumentID, std::string(tick.update_time, 8)); // 截断输出，避免乱码
            return -1;
        }

        // 解析并校验时分秒范围（变量命名更清晰）
        const int hour = (tick.update_time[0] - '0') * 10 + (tick.update_time[1] - '0');
        const int minute = (tick.update_time[3] - '0') * 10 + (tick.update_time[4] - '0');
        const int second = (tick.update_time[6] - '0') * 10 + (tick.update_time[7] - '0');
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
        {
            LOG_INFO("[Tick校验失败] 时分秒范围错误 | 合约: {} | 时间: {:02d}:{:02d}:{:02d}",
                tick.InstrumentID, hour, minute, second);
            return -1;
        }

        // 校验毫秒范围（原逻辑保留）
        if (tick.updatemill < 0 || tick.updatemill > 999)
        {
            LOG_INFO("[Tick校验失败] 毫秒范围错误 | 合约: {} | 毫秒: {}",
                tick.InstrumentID, tick.updatemill);
            return -1;
        }

        int64_t day_num = 0;
        const char* day_str = (m_config.timestamp_base == "TradingDay") ? tick.TradingDay : tick.Action_Day;
        if (!strToInt64(day_str, 8, day_num)) {
            LOG_ERROR("[时间戳拼接失败] 日期非数字 | 合约: {} | 日期: {}", tick.InstrumentID, day_str);
            return -1;
        }

        int64_t timestamp = day_num * 1000000000LL;
        timestamp += hour * 10000000LL;             
        timestamp += minute * 100000LL;            
        timestamp += second * 1000LL;               
        timestamp += tick.updatemill;               

        return timestamp;
    }

    // 乱序容忍、同毫秒数据放行
    inline bool checkTimestampNew(const TickData& tick)
    {
        const int64_t currentTs = combineTimestamp(tick);
        // 先校验时间戳合法性
        if (currentTs < 0) {
            LOG_WARN("[Tick校验失败] 时间戳拼接失败 | 合约: {}", tick.InstrumentID);
            return false;
        }

        // 同毫秒数据：配置允许则直接通过（不更新时间戳）
        if (currentTs == m_lastTimestamp && m_config.allow_same_ms_data) {
            return true;
        }

        // 乱序校验
        if (currentTs < m_lastTimestamp) {
            int64_t totalMsDiff = m_lastTimestamp - currentTs;
            if (totalMsDiff > m_config.max_disorder_tolerance_ms) {
                LOG_WARN("[Tick校验失败] 时间戳非递增 | 合约: {} | 当前: {} | 上次: {} | 差值: {}ms",
                    tick.InstrumentID, currentTs, m_lastTimestamp, totalMsDiff);
                return false;
            }
            // 轻微乱序：通过但不更新时间戳
            return true;
        }

        // 正常递增：更新时间戳
        m_lastTimestamp = currentTs;
        return true;
    }

    // 涨跌停容忍、校验档位、倒挂容忍
    inline bool checkPriceValidity(const TickData& tick) const
    {
        if (isInvalidPrice(tick.last_price)) {
            LOG_INFO("[Tick校验失败] 最新价无效 | 合约: {} | 最新价: {}", tick.InstrumentID, tick.last_price);
            return false;
        }
        if (isInvalidPrice(tick.limit_up)) {
            LOG_INFO("[Tick校验失败] 涨停价无效 | 合约: {} | 涨停价: {}", tick.InstrumentID, tick.limit_up);
            return false;
        }
        if (isInvalidPrice(tick.limit_down)) {
            LOG_INFO("[Tick校验失败] 跌停价无效 | 合约: {} | 跌停价: {}", tick.InstrumentID, tick.limit_down);
            return false;
        }
        if (!m_config.skip_zero_limit_price || (tick.limit_up > 0 && tick.limit_down > 0))
        {
            double tolerance = tick.limit_up * m_config.limit_price_tolerance_ratio;
            if (tick.last_price < (tick.limit_down - tolerance) || tick.last_price >(tick.limit_up + tolerance))
            {
                LOG_INFO("[Tick校验失败] 最新价超出涨跌停范围 | 合约: {} | 最新价: {} | 涨跌停: [{}-{}]",
                    tick.InstrumentID, tick.last_price, tick.limit_down, tick.limit_up);
                return false;
            }
        }

        int checkLevels = std::min(m_config.check_bid_ask_levels, MAX_BID_ASK_LEVELS);
        // 买盘档位逻辑
        for (int i = 1; i < checkLevels; ++i)
        {
            if (isInvalidPrice(tick.bid_price[i]) || isInvalidPrice(tick.bid_price[i - 1]))
                continue;
            if (tick.bid_price[i] == 0 || tick.bid_price[i - 1] == 0) continue;
            if (tick.bid_price[i] > tick.bid_price[i - 1])
            {
                LOG_INFO("[Tick校验失败] 买盘档位逻辑错误 | 合约: {} | 买{}价: {} | 买{}价: {}",
                    tick.InstrumentID, i + 1, tick.bid_price[i], i, tick.bid_price[i - 1]);
                return false;
            }
        }
        // 卖盘档位逻辑
        for (int i = 1; i < checkLevels; ++i)
        {
            if (isInvalidPrice(tick.ask_price[i]) || isInvalidPrice(tick.ask_price[i - 1]))
                continue;
            if (tick.ask_price[i] == 0 || tick.ask_price[i - 1] == 0) continue;
            if (tick.ask_price[i] < tick.ask_price[i - 1])
            {
                LOG_INFO("[Tick校验失败] 卖盘档位逻辑错误 | 合约: {} | 卖{}价: {} | 卖{}价: {}",
                    tick.InstrumentID, i + 1, tick.ask_price[i], i, tick.ask_price[i - 1]);
                return false;
            }
        }
        // 同档位买卖盘倒挂校验
        for (int i = 0; i < checkLevels; ++i)
        {
            if (tick.bid_price[i] == 0 || tick.ask_price[i] == 0) continue;
            // 计算允许的最大倒挂价差 = 容忍tick数 * 最小变动单位
            double maxAllowInvertSpread = m_config.max_invert_spread_tick * m_config.tick_size;
            // 实际倒挂价差（买价 - 卖价，正数表示倒挂）
            double actualInvertSpread = tick.bid_price[i] - tick.ask_price[i];
            if (actualInvertSpread > maxAllowInvertSpread + 1e-8)
            {
                LOG_INFO("[Tick校验失败] 买卖盘价格倒挂超出容忍范围 | 合约: {} | 买{}价: {} | 卖{}价: {} | 容忍最大倒挂: {} | 实际倒挂: {}",
                    tick.InstrumentID, i + 1, tick.bid_price[i], i + 1, tick.ask_price[i], maxAllowInvertSpread, actualInvertSpread);
                return false;
            }
        }
        return true;
    }

    inline bool checkVolumeValidity(const TickData& tick) const
    {
        if (tick.volume < 0)
        {
            LOG_INFO("[Tick校验失败] 总成交量为负 | 合约: {} | 成交量: {}", tick.InstrumentID, tick.volume);
            return false;
        }
        int checkLevels = std::min(m_config.check_bid_ask_levels, 5);
        for (int i = 0; i < checkLevels; ++i)
        {
            if (tick.bid_volume[i] < 0)
            {
                LOG_INFO("[Tick校验失败] 买盘量为负 | 合约: {} | 档位: {} | 量: {}",
                    tick.InstrumentID, i + 1, tick.bid_volume[i]);
                return false;
            }
            if (tick.ask_volume[i] < 0)
            {
                LOG_INFO("[Tick校验失败] 卖盘量为负 | 合约: {} | 档位: {} | 量: {}",
                    tick.InstrumentID, i + 1, tick.ask_volume[i]);
                return false;
            }
        }
        return true;
    }

    inline bool checkAbnormalPacket(const TickData& tick) const
    {
        bool isAllZero = (tick.last_price == 0) &&
            (tick.bid_price[0] == 0 && tick.ask_price[0] == 0) &&
            (tick.volume == 0) &&
            (tick.bid_volume[0] == 0 && tick.ask_volume[0] == 0);
        if (isAllZero) {
            LOG_INFO("[Tick校验失败] 异常空包/测试包 | 合约: {}", tick.InstrumentID);
            return false;
        }
        return true;
    }

public:
    // ========== 构造函数：传入合约名+校验配置 ==========
    UnitProcessor(const std::string& contractName, const json& globalRules, const std::optional<json>& contractRules)
        : m_contractName(contractName)
        , m_config(SpecialConfig::fromJson(globalRules, contractRules))
    {
        LOG_INFO("[UnitProcessor初始化] 合约: {} 校验规则加载完成", contractName);
    }

    // 配置化：按开关执行校验，关闭的校验直接跳过
    bool validate(const TickData& tick)
    {
		if (m_config.enable_exchange_check && !CheckWhiteList(tick)) return false;
        if (m_config.enable_timestamp_check && !checkTimestampNew(tick)) return false;
        if (m_config.enable_price_check && !checkPriceValidity(tick)) return false;
        if (m_config.enable_volume_check && !checkVolumeValidity(tick)) return false;
        if (m_config.enable_abnormal_packet_check && !checkAbnormalPacket(tick)) return false;
        return true;
    }

    void reset()
    {
        LOG_INFO("[UnitProcessor重置] 合约: {} 清空时间戳缓存 | 上次时间戳: {}", m_contractName, m_lastTimestamp);
        m_lastTimestamp = 0;
    }

    const std::string& getContractName() const{ return m_contractName; }

    const asio::ip::udp::endpoint& getMulticastEndpoint() const{ return m_config.m_udpConfig.target_endpoint; }
};
