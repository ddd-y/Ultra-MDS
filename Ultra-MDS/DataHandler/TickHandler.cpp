#include "TickHandler.h"
#include"Logger.h"
#include"UnitProcessor.h"
#include"tick_data.pb.h"
#include <immintrin.h>



void TickHandler::processLoop()
{
    while (my_running.load(std::memory_order_relaxed)) {
        // try_pop 是非阻塞的
        if (!my_queue_Tick->empty()) 
        {
			TickData* currentdata = my_queue_Tick->front();
            HandleTick(*currentdata);
            my_queue_Tick->pop();
        }
        else
        {
            _mm_pause();
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
        udp_sender_.SendMessage(Tick);
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

void UdpSender::initUdpMulticast()
{
    try {
        // 打开UDP Socket（IPv4）
        udp_socket_.open(udp::v4());
        // 允许端口复用（多个进程绑定同一端口）
        udp_socket_.set_option(asio::socket_base::reuse_address(true));
        LOG_INFO("UDP组播初始化成功");
    }
    catch (const std::exception& e) {
        LOG_ERROR("UDP组播初始化失败 | 错误信息：{}", e.what());
        throw std::runtime_error("UDP multicast init failed: " + std::string(e.what()));
    }
}

inline void UdpSender::SendMessage(const TickData& Tick)
{
    try {
        // 初始化ProtoBuf对象
        TickDataMes mes;

        // 核心修改：调用TickData的成员函数填充数据
        Tick.fillProtoData(mes);

        // 序列化ProtoBuf数据到字符串
        std::string proto_serialized;
        if (!mes.SerializeToString(&proto_serialized)) {
            LOG_ERROR("合约{} ProtoBuf序列化失败", Tick.InstrumentID);
            return;
        }

        // 同步发送UDP组播数据
        auto target_endpoint = Tick.m_detector->getMulticastEndpoint();
        size_t sent_bytes = udp_socket_.send_to(
            asio::buffer(proto_serialized),  // 待发送数据缓冲区
            target_endpoint                  // 组播目标端点
        );

        auto tnow = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();

        auto tduration = tnow - Tick.local_receive_time;
        LOG_INFO("合约{},从收到包到检测完发包用了{} 纳秒", Tick.InstrumentID, tduration);
    }
    catch (const std::exception& e) {
        LOG_ERROR("合约{} UDP组播发送失败 | 错误信息：{}", Tick.InstrumentID, e.what());
    }
}
