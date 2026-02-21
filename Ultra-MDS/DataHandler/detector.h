// detector.h 完整改造后代码
#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <optional>
#include "TickData.h"
#include "Logger.h"
#include "../Tool/nlohmann/json.hpp"

using json = nlohmann::json;

// ========== 新增：校验规则结构体，所有配置项带默认值 ==========
struct ValidationConfig
{
    // 校验总开关
    bool enable_base_check = true;
    bool enable_time_check = true;
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
    double limit_price_tolerance_ratio = 0.0001;
    bool skip_zero_limit_price = true;

    // 从JSON加载配置（合约专属规则覆盖全局默认）
    static ValidationConfig fromJson(const json& globalJson, const std::optional<json>& contractJson) {
        ValidationConfig config;

        // 先加载全局默认规则
        if (!globalJson.is_null()) {
            config.enable_base_check = globalJson.value("enable_base_check", config.enable_base_check);
            config.enable_time_check = globalJson.value("enable_time_check", config.enable_time_check);
            config.enable_timestamp_check = globalJson.value("enable_timestamp_check", config.enable_timestamp_check);
            config.enable_price_check = globalJson.value("enable_price_check", config.enable_price_check);
            config.enable_volume_check = globalJson.value("enable_volume_check", config.enable_volume_check);
            config.enable_abnormal_packet_check = globalJson.value("enable_abnormal_packet_check", config.enable_abnormal_packet_check);

            config.timestamp_base = globalJson.value("timestamp_base", config.timestamp_base);
            config.max_disorder_tolerance_ms = globalJson.value("max_disorder_tolerance_ms", config.max_disorder_tolerance_ms);
            config.allow_same_ms_data = globalJson.value("allow_same_ms_data", config.allow_same_ms_data);

            config.check_bid_ask_levels = std::clamp(globalJson.value("check_bid_ask_levels", config.check_bid_ask_levels), 1, 5);
            config.max_invert_spread_tick = globalJson.value("max_invert_spread_tick", config.max_invert_spread_tick);
            config.limit_price_tolerance_ratio = globalJson.value("limit_price_tolerance_ratio", config.limit_price_tolerance_ratio);
            config.skip_zero_limit_price = globalJson.value("skip_zero_limit_price", config.skip_zero_limit_price);
        }

        // 再用合约专属规则覆盖
        if (contractJson.has_value() && !contractJson.value().is_null()) {
            const json& cJson = contractJson.value();
            if (cJson.contains("enable_base_check")) config.enable_base_check = cJson["enable_base_check"];
            if (cJson.contains("enable_time_check")) config.enable_time_check = cJson["enable_time_check"];
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
            if (cJson.contains("limit_price_tolerance_ratio")) config.limit_price_tolerance_ratio = cJson["limit_price_tolerance_ratio"];
            if (cJson.contains("skip_zero_limit_price")) config.skip_zero_limit_price = cJson["skip_zero_limit_price"];
        }

        return config;
    }
};

// 针对单个合约的行情校验器，每个合约独立一个实例，由Dispatcher分发调用
class Detector
{
private:
    int64_t m_lastTimestamp = 0;
    std::string m_contractName; // 绑定的合约名称
    ValidationConfig m_config;   // 该合约专属的校验配置

    // -------------------------- 原有校验函数，适配配置化改造 --------------------------
    inline bool checkBaseValidity(const TickData& tick) const
    {
        if (strnlen(tick.InstrumentID, sizeof(tick.InstrumentID)) == 0)
        {
            LOG_INFO("[Tick校验失败] 合约代码为空 | 原始数据: InstrumentID={}", tick.InstrumentID);
            return false;
        }
        if (strnlen(tick.ExchangeID, sizeof(tick.ExchangeID)) == 0)
        {
            LOG_INFO("[Tick校验失败] 交易所代码为空 | 合约: {}", tick.InstrumentID);
            return false;
        }
        const size_t dayLen = strnlen(tick.Action_Day, sizeof(tick.Action_Day));
        if (dayLen != 8)
        {
            LOG_INFO("[Tick校验失败] 业务日期格式错误 | 合约: {} | 日期: {}",
                tick.InstrumentID, std::string(tick.Action_Day, dayLen));
            return false;
        }
        const size_t timeLen = strnlen(tick.update_time, sizeof(tick.update_time));
        if (timeLen != 8)
        {
            LOG_INFO("[Tick校验失败] 时间字段格式错误 | 合约: {} | 时间: {}",
                tick.InstrumentID, std::string(tick.update_time, timeLen));
            return false;
        }
        return true;
    }

    inline bool checkTimeValidity(const TickData& tick) const
    {
        if (tick.update_time[2] != ':' || tick.update_time[5] != ':')
        {
            LOG_INFO("[Tick校验失败] 时间格式错误（冒号位置不对） | 合约: {} | 时间: {}",
                tick.InstrumentID, tick.update_time);
            return false;
        }
        const int hour = (tick.update_time[0] - '0') * 10 + (tick.update_time[1] - '0');
        const int minute = (tick.update_time[3] - '0') * 10 + (tick.update_time[4] - '0');
        const int second = (tick.update_time[6] - '0') * 10 + (tick.update_time[7] - '0');
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
        {
            LOG_INFO("[Tick校验失败] 时分秒范围错误 | 合约: {} | 时间: {:02d}:{:02d}:{:02d}",
                tick.InstrumentID, hour, minute, second);
            return false;
        }
        if (tick.updatemill < 0 || tick.updatemill > 999)
        {
            LOG_INFO("[Tick校验失败] 毫秒范围错误 | 合约: {} | 毫秒: {}",
                tick.InstrumentID, tick.updatemill);
            return false;
        }
        return true;
    }

    // 配置化：时间戳基准切换（ActionDay/TradingDay）
    inline int64_t combineTimestamp(const TickData& tick) const
    {
        char timestampBuf[18] = { 0 };
        // 按配置选择时间戳基准（适配夜盘行情）
        if (m_config.timestamp_base == "TradingDay") {
            memcpy(timestampBuf, tick.TradingDay, 8);
        }
        else {
            memcpy(timestampBuf, tick.Action_Day, 8);
        }
        timestampBuf[8] = tick.update_time[0];
        timestampBuf[9] = tick.update_time[1];
        timestampBuf[10] = tick.update_time[3];
        timestampBuf[11] = tick.update_time[4];
        timestampBuf[12] = tick.update_time[6];
        timestampBuf[13] = tick.update_time[7];
        sprintf(timestampBuf + 14, "%03d", tick.updatemill);
        return atoll(timestampBuf);
    }

    // 配置化：乱序容忍、同毫秒数据放行
    inline bool checkTimestampNew(const TickData& tick)
    {
        const int64_t currentTs = combineTimestamp(tick);
        // 同毫秒数据：配置允许则直接通过
        if (currentTs == m_lastTimestamp && m_config.allow_same_ms_data) {
            return true;
        }
        // 轻微乱序容忍：在阈值内放行
        int64_t msDiff = (currentTs % 1000) - (m_lastTimestamp % 1000);
        if (currentTs < m_lastTimestamp && llabs(msDiff) <= m_config.max_disorder_tolerance_ms) {
            return true;
        }
        // 核心时序判断
        if (currentTs > m_lastTimestamp) {
            m_lastTimestamp = currentTs;
            return true;
        }
        LOG_WARN("[Tick校验失败] 时间戳非递增 | 合约: {} | 当前: {} | 上次: {}",
            tick.InstrumentID, currentTs, m_lastTimestamp);
        return false;
    }

    // 配置化：涨跌停容忍、校验档位、倒挂容忍
    inline bool checkPriceValidity(const TickData& tick) const
    {
        auto isInvalidPrice = [](double price) -> bool {
            return std::isnan(price) || std::isinf(price) || price < 0;
            };
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

        // 配置化：涨跌停范围校验，带容忍度
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

        // 配置化：按配置的档位校验买卖盘逻辑
        int checkLevels = std::min(m_config.check_bid_ask_levels, 5);
        // 买盘档位逻辑
        for (int i = 1; i < checkLevels; ++i)
        {
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
            if (tick.bid_price[i] > tick.ask_price[i])
            {
                LOG_INFO("[Tick校验失败] 买卖盘价格倒挂 | 合约: {} | 买{}价: {} | 卖{}价: {}",
                    tick.InstrumentID, i + 1, tick.bid_price[i], i + 1, tick.ask_price[i]);
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
        if (tick.last_price == 0 && tick.bid_price[0] == 0 && tick.ask_price[0] == 0 && tick.volume == 0)
        {
            LOG_INFO("[Tick校验失败] 异常空包/测试包 | 合约: {}", tick.InstrumentID);
            return false;
        }
        return true;
    }

public:
    // ========== 改造构造函数：传入合约名+校验配置 ==========
    Detector(const std::string& contractName, const json& globalRules, const std::optional<json>& contractRules)
        : m_contractName(contractName)
        , m_config(ValidationConfig::fromJson(globalRules, contractRules))
    {
        LOG_INFO("[Detector初始化] 合约: {} 校验规则加载完成", contractName);
    }

    // 配置化：按开关执行校验，关闭的校验直接跳过
    bool validate(const TickData& tick)
    {
        if (m_config.enable_base_check && !checkBaseValidity(tick)) return false;
        if (m_config.enable_time_check && !checkTimeValidity(tick)) return false;
        if (m_config.enable_timestamp_check && !checkTimestampNew(tick)) return false;
        if (m_config.enable_price_check && !checkPriceValidity(tick)) return false;
        if (m_config.enable_volume_check && !checkVolumeValidity(tick)) return false;
        if (m_config.enable_abnormal_packet_check && !checkAbnormalPacket(tick)) return false;
        return true;
    }

    void reset()
    {
        LOG_INFO("[Detector重置] 合约: {} 清空时间戳缓存 | 上次时间戳: {}", m_contractName, m_lastTimestamp);
        m_lastTimestamp = 0;
    }
};
