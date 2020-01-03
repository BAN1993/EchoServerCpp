#pragma once

/*=============================================
 =	监听Socket

 +	NOTE
		1.由主线程调用循环
		2.会创建N个io线程处理连接

==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <socket/SocketBase.h>
#include <base/Lock.h>

class Server;
class ServerSocketIOThread;
class ServerIOSocket;
class AgentClient;

class ServerSocketCallBack
{
public:
	/**
	 * 收到新的客户端连接
	 */
	virtual void onAcceptClient(AgentClient* client) = 0;

	/**
	 * 收到消息
	 * 每次一个包,且保证包的完整性
	 * head : 包头
	 * buf : 整包(包头+包体)
	 * len : 整包长
	 */
	virtual void onClientRecive(AgentClient* client, struct ProtocolHead head, char* buf, unsigned int len) = 0;

	/**
	 * 客户端关闭连接
	 */
	virtual void onClientClose(AgentClient* client) = 0;

	/**
	 * 计时器,每秒(不完全精确)
	 */
	virtual void onTimer() = 0;

	/**
	 * 帧计时器
	 * 时间间隔由注册函数指定
	 * TODO 这个实现不合理,先这么做吧
	 */
	virtual void onFrameTimer() = 0;
};

class ServerSocket : public unCopyable
{
	friend class ServerSocketIOThread;
	friend class ServerIOSocket;
	friend class AgentClient;

public:
	ServerSocket(int port, ServerSocketCallBack* cb, unsigned int threadcnt);
	~ServerSocket();

public:
	inline ServerSocketCallBack* getCB() { return _pCB; };
	inline event_base* getBase(){ return _pBase; };
	inline void setMaxSocket(int num)		{ _nMaxSockets = num; };
	inline void setPairDealCount(int num)	{ _nPairDealCount = num; };
	inline void setTimeOut(int sec)			{ _nIOSocketTimeOut = sec; };

public:
	/**
	 * 初始化监听,开启线程循环
	 */
	int begin();

	/**
	 * 注册帧计时器
	 * 时间单位毫秒
	 */
	int registFrameTimer(unsigned int msec);

private:
	/**
	 * io线程->主线程
	 * 通知时底层已经初始化完成了
	 * 现在还没想好普通tcp连接的加密放在哪里做，如果放在底层那么到这里时加密已经完成了
	 */
	int asynAccept(ServerIOSocket* socket);

	/**
	 * io线程->主线程
	 * 如果是websocket,底层已经处理过握手了,不会上抛
	 * 收到的是个完整包，不会有半个或多个的情况，放心使用
	 * 注意buf要delete
	 */
	int asynRecive(ServerIOSocket* socket, char* buf, unsigned int len);

	/**
	 * io线程->主线程
	 * 通知时底层还没做任何关闭或清理动作
	 * 所以等上层数据清理完成后还需要再通知回io线程
	 */
	int asynClose(ServerIOSocket* socket);

	/**
	 * event回调,收到新连接
	 * 将收到的连接轮流分配给各个io线程
	 */
	static void acceptCallback(evutil_socket_t listensocket, short event, void *arg);

	/**
	 * event回调,收到线程数据
	 * 消耗 _listPackList
	 * NOTE 是每次消耗一个好还是直接消耗完呢? 现在是每次尽量消耗完,但有上限
	 */
	static void pairCallback(evutil_socket_t fd, short event, void *arg);

	/**
	 * event回调,计时器
	 * 每秒一次
	 */
	static void timerCallback(evutil_socket_t fd, short event, void *arg);

	/**
	 * event回调,帧计时器
	 * 根据注册函数来确定时间间隔
	 */
	static void timerFrameCallback(evutil_socket_t fd, short event, void *arg);

	/**
	 * 初始化线程交互的管道
	 */
	int initSocketPair();

	/**
	 * 主线程主动调用关闭连接
	 * 现在这种方式好像不太合理
	 */
	int closeByLogic(AgentClient* client);

	AgentClient* getAgent(ServerIOSocket* socket);
	void deleteAgent(AgentClient* agent);

private:
	/**
	 * 其中有一些是配置
	 * 为什么不直接从ServerConfig获取?
	 * 因为未来打算写成socket库,不希望和外界代码有联系
	 */
	int _nListenSocketFd;
	int _nPort;
	int _nIOSocketTimeOut;		// 超时时间
	int _nMaxSockets;			// 最大连接数
	int _nNowSocketCount;		// 当前客户端连接数
	int _nPairDealCount;		// 每次最多处理多少线程数据

	ServerSocketCallBack* _pCB;
	event* _pListenEvent;
	event_base* _pBase;
	event* _pTimerEvent;
	event* _pFrameEvent;

	std::vector<ServerSocketIOThread*> _vIOThreads;	// 线程列表
	int _nThreadCount;								// 线程数量
	int _nCurrentThread;							// 等待分配的游标

	Lock _lock;
	event* _pThreadEvent;
	evutil_socket_t _fdPair[2]; // 线程通信socket
	std::queue<threadPackage> _listPackList; // 线程通信队列

	std::map<unsigned long long, AgentClient*> _mapAgents;

	int _testFlag; // 测试标识,开启gperftools
};
