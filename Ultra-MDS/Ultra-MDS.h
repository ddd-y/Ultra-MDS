// Ultra-MDS.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include"configrelated/Loginer.h"
#include"SystemStart.h"

// Ultra-MDS 应用程序的入口点
void AppStart() 
{
	SystemStarter Starter;
	while (true) {
		pause(); 
	}
}
