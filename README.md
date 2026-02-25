# CTP 高性能行情转发器

**Ultra-MDS** 是一款基于 CTP 接口开发的**低延迟、高可靠、可配置化**期货行情转发器，专为量化交易场景设计。
通过无锁队列实现行情数据的极速流转，内置多维度行情合规校验，支持按合约独立配置 UDP 组播转发，兼顾灵活性与极致性能。

---

## ✨ 核心特性

| 特性 | 说明 |
|------|------|
| 🚀 **低延迟** | 基于 SPSC 无锁队列实现行情流转，在腾讯云2核2GB内存的云服务器上实现了微秒级的行情校验及转发 |
| 🛡️ **多维度行情校验** | 内置交易所白名单、时间戳乱序校验、价格合法性、买卖盘倒挂、成交量合规、异常空包检测等全维度校验 |
| ⚙️ **灵活配置体系** | 「全局默认规则 + 合约专属规则」双层配置，支持按合约独立设置校验规则、UDP 组播地址/端口 |
| 📡 **多合约独立转发** | 不同合约可转发至不同 UDP 组播地址，满足多策略、多节点的行情分发需求 |
| 📝 **可观测性日志** | 校验失败、配置错误、转发异常均输出明确的上下文日志，快速定位问题 |

## 📖 完整配置说明

Ultra-MDS 采用 JSON 格式配置文件（默认 `myconfig.json`），支持「全局默认规则 + 合约专属规则」双层配置体系，合约专属规则会覆盖全局默认规则，未配置的字段自动继承全局值。

### 1. 配置文件整体结构

```json
{
  "login": { /* CTP 登录信息 */ },
  "subscribed_contracts": [ /* 订阅合约列表 */ ],
  "validation": {
    "global_default": { /* 全局默认校验&转发规则 */ },
    "contract_specific": { /* 合约专属规则（按需配置） */ }
  },
  "system": { /* 系统级配置（可选） */ }
}
```

## ⚙️ 配置字段说明
配置位于 `validation` 节点下，支持「全局默认规则 `global_default` + 合约专属规则 `contract_specific`」双层配置，合约专属规则会覆盖全局默认值。

### 📡 UDP 组播配置
| 字段名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `port` | int | `8888` | UDP 组播转发端口 |
| `Multi_add` | string | `"239.0.0.1"` | UDP 组播地址 |

### 🛡️ 校验总开关
| 字段名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `enable_exchange_check` | bool | `true` | 是否开启交易所白名单校验 |
| `enable_timestamp_check` | bool | `true` | 是否开启时间戳合法性 / 乱序校验 |
| `enable_price_check` | bool | `true` | 是否开启价格有效性校验 |
| `enable_volume_check` | bool | `true` | 是否开启成交量合法性校验 |
| `enable_abnormal_packet_check` | bool | `true` | 是否开启异常空包检测 |

### ⏱️ 时间戳配置
| 字段名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `timestamp_base` | string | `"TradingDay"` | 时间戳基准（可选值：`TradingDay` / `ActionDay`） |
| `max_disorder_tolerance_ms` | int | `10` | 时间戳乱序最大容忍毫秒数 |
| `allow_same_ms_data` | bool | `true` | 是否允许相同毫秒的行情数据 |

### 📈 价格 / 买卖盘配置
| 字段名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `check_bid_ask_levels` | int | `1` | 校验的买卖盘档位数量（取值范围 1-5） |
| `max_invert_spread_tick` | int | `1` | 买卖盘倒挂最大容忍 Tick 数 |
| `tick_size` | double | `1.0` | 合约最小变动单位（<=0 会自动重置为 1.0） |
| `limit_price_tolerance_ratio` | double | `0.0001` | 涨跌停价格容忍比例 |
| `skip_zero_limit_price` | bool | `true` | 是否跳过涨跌停价为 0 的校验 |

### 📋 白名单配置
| 字段名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `exchange_whitelist` | array | `[]` | 交易所白名单（空列表默认放行所有，示例：`["SHFE", "CFFEX"]`） |

---

### 合约专属配置示例
在 `contract_specific` 中，仅需填写与全局默认不同的字段即可，无需重复配置全部内容：
```json
"contract_specific": {
  "IF2603": {
    "port": 8889,
    "check_bid_ask_levels": 5,
    "tick_size": 0.2
  },
  "cu2605": {
    "enable_volume_check": false
  }
}
```
## 📡 行情数据结构说明 (Protobuf)
本项目基于 **Protocol Buffers 3 (proto3)** 定义标准化行情数据结构 `TickDataMes`，用于UDP组播场景下的高效序列化与跨语言数据交互。

### 💰 基础价格字段
| 字段名 | 类型 | 字段编号 | 说明 |
| :--- | :--- | :--- | :--- |
| `last_price` | double | `1` | 最新成交价 |
| `bid_price` | repeated double | `2` | 买方报价列表（买一至买五） |
| `ask_price` | repeated double | `3` | 卖方报价列表（卖一至卖五） |
| `limit_up` | double | `4` | 合约涨停价 |
| `limit_down` | double | `5` | 合约跌停价 |

### 📊 成交量与报量字段
| 字段名 | 类型 | 字段编号 | 说明 |
| :--- | :--- | :--- | :--- |
| `volume` | int32 | `6` | 合约累计成交量 |
| `updatemill` | int32 | `7` | 行情数据更新毫秒数 |
| `bid_volume` | repeated int32 | `8` | 买方报量列表（对应bid_price档位） |
| `ask_volume` | repeated int32 | `9` | 卖方报量列表（对应ask_price档位） |

### 📋 合约与时间信息字段
| 字段名 | 类型 | 字段编号 | 说明 |
| :--- | :--- | :--- | :--- |
| `InstrumentID` | string | `10` | 合约代码（如 `rb2605`、`IF2603`） |
| `ExchangeID` | string | `11` | 交易所代码（如 `SHFE`、`CFFEX`） |
| `update_time` | string | `12` | 行情更新时间（格式：`HH:MM:SS`） |
| `Action_Day` | string | `13` | 交易日（格式：`YYYYMMDD`） |
| `TradingDay` | string | `14` | 交易日期（格式：`YYYYMMDD`） |

---
