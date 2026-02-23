#pragma once
#include<string>
#include <thread>
#include"ThostFtdcMdApi.h"
#include"ThostFtdcTraderApi.h"
#include"Logger.h"



constexpr const int MAX_RETRY = 3;
//登录信息存储类
class Loginer
{
public:

	static Loginer& getInstance() { return instance; }

    void setBrokerId(const std::string& n_id) { brokerID = n_id; }
    void setUserId(const std::string& n_id) { userID = n_id; }
    void setPassword(const std::string& n_word) { password = n_word; }
    void setTcpAddress(const std::string& n_adress) { tcpAddress = n_adress; }

    std::string getTcpAddress() { return tcpAddress; }
    //Login无需空指针检查，检查在外面做
    static int Login(CThostFtdcMdApi* pApi) {

        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        auto& loginer = instance;

        strncpy(req.BrokerID, loginer.brokerID.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.UserID, loginer.userID.c_str(), sizeof(req.UserID) - 1);
        strncpy(req.Password, loginer.password.c_str(), sizeof(req.Password) - 1);

        int nResult = -1;
        int retryCount = 0;


        while (retryCount < MAX_RETRY)
        {
            nResult = pApi->ReqUserLogin(&req, 0);

            if (nResult == 0) {
                LOG_INFO("【行情接口】登录请求发送成功。");
                break;
            }
            else {
                // 优化：错误码说明更清晰
                std::string errDesc = "";
                switch (nResult) {
                case -1: errDesc = "网络连接丢失"; break;
                case -2: errDesc = "未处理请求超限"; break;
                case -3: errDesc = "流量控制（发送频率太快）"; break;
                default: errDesc = "未知错误"; break;
                }
                retryCount++;
                LOG_WARN("【行情接口】ReqUserLogin 失败, error code: {}({}), retry {}/{}",
                    nResult, errDesc, retryCount, MAX_RETRY);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }

        if (nResult != 0)
        {
            LOG_ERROR("【行情接口】{} 次尝试发送登录请求依旧失败", MAX_RETRY);
            return nResult;
        }
        return nResult;
    }
private:
	static Loginer instance;
	Loginer() = default;
	Loginer(const Loginer&) = delete;

    std::string brokerID = "";
    std::string userID = "";
    std::string password = "";
    std::string tcpAddress = "";

};