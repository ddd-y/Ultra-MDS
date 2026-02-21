#include "TickHandler.h"
#include"Logger.h"
#include"detector.h"
#include"tick_data.pb.h"


std::string TickHandler::multicast_addr_;
uint16_t TickHandler::multicast_port_;

void TickHandler::processLoop()
{
    while (my_running.load(std::memory_order_relaxed)) {
        // try_pop 是非阻塞的
        if (TickData *currentdata=my_queue_Tick->front()) 
        {
            HandleTick(*currentdata);
            my_queue_Tick->pop();
        }
    }
}

void TickHandler::setThreadAffinity(std::thread& t, int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);          // 清空核心集合
    CPU_SET(core_id, &cpuset);  // 将目标核心加入集合

    // 获取std::thread的底层pthread句柄
    pthread_t thread_handle = t.native_handle();
    int ret = pthread_setaffinity_np(thread_handle, sizeof(cpu_set_t), &cpuset);

    // 错误处理
    if (ret != 0) {
        LOG_ERROR("TickHandler绑定核心{}失败，错误码：{}", core_id, ret);
        throw std::runtime_error("线程核心绑定失败");
    }
    LOG_INFO("TickHandler线程成功绑定到核心{}", core_id);
}


inline void TickHandler::HandleTick(const TickData& Tick)
{
    if (Tick.m_detector->validate(Tick))
    {
		SendMessage(Tick);
    }
}

void TickHandler::initUdpMulticast()
{
    try {
        // 解析组播地址和端口
        multicast_endpoint_ = udp::endpoint(
            asio::ip::make_address(multicast_addr_),
            multicast_port_
        );

        // 打开UDP Socket（IPv4）
        udp_socket_.open(udp::v4());

        // 允许端口复用（多个进程绑定同一端口）
        udp_socket_.set_option(asio::socket_base::reuse_address(true));

        LOG_INFO("UDP组播初始化成功 | 组播地址：{}:{}", multicast_addr_, multicast_port_);
    }
    catch (const std::exception& e) {
        LOG_ERROR("UDP组播初始化失败 | 错误信息：{}", e.what());
        throw std::runtime_error("UDP multicast init failed: " + std::string(e.what()));
    }
}

inline void TickHandler::SendMessage(const TickData& Tick)
{
    try {
        // 完整填充ProtoBuf数据
        TickDataMes mes;

        // 基础价格字段
        mes.set_last_price(Tick.last_price);
        mes.set_limit_up(Tick.limit_up);
        mes.set_limit_down(Tick.limit_down);

        // 五档买价（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_bid_price(Tick.bid_price[i]);
        }

        // 五档卖价（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_ask_price(Tick.ask_price[i]);
        }

        // 成交量、更新毫秒
        mes.set_volume(Tick.volume);
        mes.set_updatemill(Tick.updatemill);

        // 五档买量（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_bid_volume(Tick.bid_volume[i]);
        }

        // 五档卖量（repeated字段）
        for (int i = 0; i < 5; ++i) {
            mes.add_ask_volume(Tick.ask_volume[i]);
        }

        // 其他整数字段
        mes.set_local_receive_time(Tick.local_receive_time);

        // 字符串字段
        mes.set_instrumentid(std::string(Tick.InstrumentID, strnlen(Tick.InstrumentID, sizeof(Tick.InstrumentID))));
        mes.set_exchangeid(std::string(Tick.ExchangeID, strnlen(Tick.ExchangeID, sizeof(Tick.ExchangeID))));
        mes.set_update_time(std::string(Tick.update_time, strnlen(Tick.update_time, sizeof(Tick.update_time))));
        mes.set_action_day(std::string(Tick.Action_Day, strnlen(Tick.Action_Day, sizeof(Tick.Action_Day))));
        mes.set_tradingday(std::string(Tick.TradingDay, strnlen(Tick.TradingDay, sizeof(Tick.TradingDay))));

        // 序列化ProtoBuf数据到字符串
        std::string proto_serialized;
        if (!mes.SerializeToString(&proto_serialized)) {
            LOG_ERROR("合约{} ProtoBuf序列化失败", Tick.InstrumentID);
            return;
        }

        // 3. 同步发送UDP组播数据
        size_t sent_bytes = udp_socket_.send_to(
            asio::buffer(proto_serialized),  // 待发送数据缓冲区
            multicast_endpoint_              // 组播目标端点
        );

		auto tnow = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();

        auto tduration = tnow - Tick.local_receive_time;
        LOG_INFO("合约{},从收到包到检测完发包用了{} 纳秒", Tick.InstrumentID ,tduration);
        LOG_INFO("合约{} UDP组播发送成功 | 发送字节数：{} | 总数据大小：{}",
            Tick.InstrumentID, sent_bytes, proto_serialized.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("合约{} UDP组播发送失败 | 错误信息：{}", Tick.InstrumentID, e.what());
    }
}

void TickHandler::start(int core_id)
{
    if (my_running) return;
    my_running = true;

    // 启动processLoop线程并绑定到core_id
    my_worker_thread_Tick = std::thread(&TickHandler::processLoop, this);
    setThreadAffinity(my_worker_thread_Tick, core_id);
}

void TickHandler::stop()
{
    my_running = false;
    // 等待processLoop线程退出
    if (my_worker_thread_Tick.joinable()) {
        my_worker_thread_Tick.join();
    }
    delete my_queue_Tick;
}
