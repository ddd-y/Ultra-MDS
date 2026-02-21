#pragma once

#include"SPSCQueue.h"
#include<thread>
#include"TickData.h"
#include <unistd.h>   // sysconf（获取核心数）
#include <pthread.h>  // pthread_setaffinity_np（核心绑定）
#include <sched.h>    // cpu_set_t、CPU_ZERO/CPU_SET（核心集合操作）
#include <stdexcept>  // 异常处理（可选，增强鲁棒性）
#include <boost/asio.hpp>

namespace asio = boost::asio;
using udp = asio::ip::udp;

class TickHandler {
private:
    rigtorp::SPSCQueue<TickData>* my_queue_Tick;
    std::atomic<bool> my_running{ false }; // 线程运行标志

    std::thread my_worker_thread_Tick;        // 线程句柄

    asio::io_context io_context_;          // Asio IO上下文
    udp::socket udp_socket_;               
    udp::endpoint multicast_endpoint_;     // 组播目标端点
    static std::string multicast_addr_; // 组播地址（D类地址：224.0.0.0-239.255.255.255）
    static uint16_t multicast_port_;          // 组播端口


    void processLoop();

    //设置线程的CPU亲和度
    void setThreadAffinity(std::thread& t, int core_id);

    //处理tickdata的成员函数
    inline void HandleTick(const TickData& Tick);

    void initUdpMulticast();
	inline void SendMessage(const TickData& Tick);
public:
    TickHandler(rigtorp::SPSCQueue<TickData>* n_queue)
       :my_queue_Tick(n_queue), udp_socket_(io_context_) {
        initUdpMulticast();
    }

    // 析构时确保线程安全退出
    ~TickHandler() { stop(); }

	void start(int core_id);//启动线程，并绑定到指定核心（core_id从0开始，需小于系统核心数）
    void stop();  // 停止线程

    rigtorp::SPSCQueue<TickData>* GetTickQueue() { return my_queue_Tick; }

	static void setMulticastAddress(const std::string& addr) { multicast_addr_ = addr; }
	static void setMulticastPort(uint16_t port) { multicast_port_ = port; }
};
