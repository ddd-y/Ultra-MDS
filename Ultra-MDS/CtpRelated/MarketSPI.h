#pragma once
#include "ThostFtdcMdApi.h"
#include<string>
#include"Logger.h"
#include<vector>


//行情SPI回调接口
class MarketSPI: public CThostFtdcMdSpi
{
public:
	virtual void OnFrontConnected() override;
	virtual void OnFrontDisconnected(int nReason) override;
	virtual void OnHeartBeatWarning(int nTimeLapse) override;
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override;
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
};



class MarketManager 
{
	//单例模式管理MarketSPI和MarketApi
private:
	MarketSPI* marketSPI=nullptr;
	CThostFtdcMdApi* marketApi=nullptr;
	static MarketManager instance;
	MarketManager()=default;
	MarketManager(const MarketManager&) = delete;
	MarketManager& operator=(const MarketManager&) = delete;

public:
	static CThostFtdcMdApi* getMarketApi() { return instance.marketApi; }
	static MarketManager& getInstance() { return instance; }
	static void Init();
};