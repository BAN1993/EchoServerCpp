#include <base/Base.h>
#include <base/Utils.h>
#include <base/Log.h>
#include "ServerConfig.h"
#include "Server.h"

/*=============================================
 + TODO
		-集成localRedis
			这个看情况吧,对性能也是种损失

==============================================*/

int main()
{
	if (!GET_SINGLE(ServerConfig)->load())
	{
		std::cout << "load serverConfig error!" << std::endl;
		return 1;
	}

	if (GET_SINGLE(ServerConfig)->getDaemon())
	{
		LOGSYS("Daemon start!");
		daemon();
	}

	initLog(
		getModulePath() + GET_SINGLE(ServerConfig)->getLogFileName(),
		GET_SINGLE(ServerConfig)->getLogFileSize(),
		GET_SINGLE(ServerConfig)->getLogLevel(),
		GET_SINGLE(ServerConfig)->getStdOut());
	
	Server s;
	if (!s.begin())
		LOGE("server begin error");
	
	// 防止有日志没打印完
	// TODO 这种设计有点不合理
	sleepMillisecond(1000);
	return 0;
}
