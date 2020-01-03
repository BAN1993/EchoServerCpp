#pragma once

/*=============================================
 =	服务配置

 +	NOTE
		1.非线程安全
		  但目前只在主线程调用,所以没事

==============================================*/

#include <tool/Uncopyable.h>
#include <tool/Singleton.h>

struct ServerStruct
{
	unsigned int id;
	std::string host;
	unsigned int port;
};

class ServerConfig : public unCopyable
{
	DEFINE_SINGLE(ServerConfig);

private:
	ServerConfig(){};
	~ServerConfig(){};

public:
	bool load();

	inline const std::string& getLogFileName()	{ return _sLogFileName; };
	inline int getLogFileSize()					{ return _nLogFileSize; };
	inline int getLogLevel()					{ return _nLogLevel; };
	inline bool getStdOut()						{ return _nStdOut; };

	inline int getServerID()					{ return _nServerID; };
	inline int getListenPort()					{ return _nListenPort; };
	inline int getServerTimeOut()				{ return _nServerTimeOut; };
	inline int getServerThreads()				{ return _nServerThreads; };
	inline int getClientThreads()				{ return _nClientThreads; };
	inline int getMaxSockets()					{ return _nMaxSockets; };
	inline int getPairDealCount()				{ return _nPairDealCount; };
	inline bool getDaemon()						{ return _nDaemon; };
	
private:
	std::string _sLogFileName;
	int _nLogFileSize;
	int _nLogLevel;
	bool _nStdOut;			// 日志是否打印到屏幕

	int _nServerID;			// 服务唯一id
	int _nListenPort;
	int _nServerTimeOut;	// 心跳超时时间
	int _nServerThreads;	// 被动连接线程数
	int _nClientThreads;	// 主动连接线程数
	int _nMaxSockets;		// 被动连接支持最大连接数
	int _nPairDealCount;	// 每次最多处理的线程数据
	bool _nDaemon;			// 是否开启守护进程
};
