// Ultra-MDS.cpp: 定义应用程序的入口点。
//

#include "Ultra-MDS.h"
//测试tick数据初始化性能的函数，包含生成模拟CTP数据和测量Protobuf对象初始化耗时的逻辑
//#include"test/tick_perf_test.h"


Dispatcher Dispatcher::instance;
Loginer Loginer::instance;
MarketManager MarketManager::instance;

int main()
{
	AppStart();
	return 0;
}
