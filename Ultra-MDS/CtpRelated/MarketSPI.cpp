#include"MarketSPI.h"
#include"memory.h"
#include"../configrelated/Loginer.h"
#include"Contract/Dispatcher.h"
#include"Contract/Container.h"



void MarketSPI::OnFrontConnected()
{
    CThostFtdcMdApi* api = MarketManager::getMarketApi();
    int LoginResult = Loginer::Login(api);
    if (LoginResult != 0) {
		LOG_INFO("MarketSPI::OnFrontConnected - 登录请求发送失败，错误码: {}", LoginResult);
    }
}

void MarketSPI::OnFrontDisconnected(int nReason)
{
    // 解析断开原因
    std::string reason_desc;
    switch (nReason)
    {
    case 0x1001: reason_desc = "网络读失败"; break;
    case 0x1002: reason_desc = "网络写失败"; break;
    case 0x2001: reason_desc = "接收心跳超时"; break;
    case 0x2002: reason_desc = "发送心跳失败"; break;
    case 0x3001: reason_desc = "服务器主动断开"; break;
    default: reason_desc = "未知原因"; break;
    }

    LOG_WARN("行情前置机断开连接 | 原因码：{} | 描述：{}", nReason, reason_desc);
}

void MarketSPI::OnHeartBeatWarning(int nTimeLapse)
{
}

void MarketSPI::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
    // 1. 登录失败处理
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        // 登录失败
        LOG_WARN("登陆失败, error code: {}, error message: {}",
			pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }

    // 2. 登录成功
    if (pRspUserLogin) {
        //获取合约容器中的所有合约
        const auto& container = ContractContainer::instance();
        const auto& contracts = container.getAllContracts();  // const std::vector<std::string>&

        if (contracts.empty()) {
            LOG_WARN("No contracts to subscribe after market login.");
            return;
        }

        std::vector<const char*> instList;
        instList.reserve(contracts.size());
        for (const auto& inst : contracts) {
            instList.push_back(inst.c_str());
        }

        auto* marketApi = MarketManager::getInstance().getMarketApi();
        int ret = marketApi->SubscribeMarketData(
            const_cast<char**>(instList.data()),
            static_cast<int>(instList.size()));

        if (ret == 0) {
            LOG_INFO("SubscribeMarketData request sent for {} contracts.", contracts.size());
        }
        else {
            LOG_ERROR("SubscribeMarketData failed, error code: {}", ret);
        }
    }
}

void MarketSPI::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
    Dispatcher::dispatch(*pDepthMarketData);
}

void MarketSPI::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void MarketSPI::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    // 先判断响应是否成功
    bool is_success = (pRspInfo == nullptr || pRspInfo->ErrorID == 0);

    std::string instrument_id;
    if (pSpecificInstrument != nullptr) {
        instrument_id = pSpecificInstrument->InstrumentID;
    }

    if (is_success) {
        if (!instrument_id.empty()) {
            LOG_INFO("✅ 订阅行情成功 | 合约代码：{} | 请求ID：{}", instrument_id, nRequestID);

        }
    }
    else {
        std::string error_msg = (pRspInfo != nullptr) ? pRspInfo->ErrorMsg : "未知错误";
        int error_id = (pRspInfo != nullptr) ? pRspInfo->ErrorID : -1;

        LOG_ERROR("❌ 订阅行情失败 | 合约代码：{} | 错误ID：{} | 错误信息：{} | 请求ID：{}",
            instrument_id.empty() ? "空" : instrument_id,
            error_id, error_msg, nRequestID);
    }
    if (bIsLast) {
        LOG_INFO("📋 本次订阅行情请求全部处理完毕 | 请求ID：{}", nRequestID);
    }
}

void MarketSPI::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	//暂时不需要处理退订响应
}

void MarketManager::Init()
{
	instance.marketSPI = new MarketSPI();
	instance.marketApi = CThostFtdcMdApi::CreateFtdcMdApi();
	instance.marketApi->RegisterSpi(instance.marketSPI);
	instance.marketApi->RegisterFront(const_cast<char*>(Loginer::getInstance().getTcpAddress().c_str()));
	instance.marketApi->Init();
	LOG_INFO("MarketManager 初始化");
}
