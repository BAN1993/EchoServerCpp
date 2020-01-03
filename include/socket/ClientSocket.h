#pragma once

/*==========================================
 = 客户端 Socket
 
 + NOTE
		1.需要传Server的base过来
		2.作为客户端时,服务端连接需要保证一直连着,
			所以一旦连接失败或失去连接就要重连

===========================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <socket/SocketBase.h>
#include <base/Lock.h>

class ClientSocketIOThread;
class ClientIOSocket;
class AgentServer;

class ClientSocketCallback
{
public:
	virtual void onConnectServer(AgentServer* server, int flag) = 0;
	virtual void onServerRecive(AgentServer* server, struct ProtocolHead head, char* buf, unsigned int len) = 0;
	virtual void onServerClose(AgentServer* server) = 0;
};

class ClientSocket : public unCopyable
{
	friend class ClientSocketIOThread;
	friend class ClientIOSocket;

public:
	ClientSocket(event_base* base, ClientSocketCallback* cb, unsigned int threadcnt);
	~ClientSocket();

public:
	inline ClientSocketCallback* getCB(){ return _pCB; };

	/**
	* 设置每次循环最多处理多少线程数据
	* 防止忙于处理数据而不触发计时器
	* 一般达到上限可能就有性能问题了
	*/
	inline void setPairDealCount(int num) { _nPairDealCount = num; };

public:
	/**
	 * 启动所有client io线程
	 */
	int begin();
	
	/**
	 * 请求连接
	 * 逻辑层调用的接口
	 */
	int connect(int id, std::string host, int port);

private:
	/**
	 * 主线程->io线程
	 * 不是真正的请求连接,只是分配一个线程然后调用异步连接
	 */
	int asynConnect(int id, std::string host, int port);

	/**
	 * io线程->主线程
	 * 连接结束.flag(0-成功,<0失败)
	 */
	int asynConnectEnd(ClientIOSocket* socket, int flag);

	/**
	 * io线程->主线程
	 * 收到数据
	 * 收到的是个完整包，不会有半个或多个的情况，放心使用
	 * 注意buf要delete
	 */
	int asynRecive(ClientIOSocket* socket, char* buf, unsigned int len);

	/**
	 * io线程->主线程
	 * 连接关闭
	 */
	int asynClose(ClientIOSocket* socket);

	/**
	 * event回调,收到线程数据
	 * 消耗 _listPackList
	 * NOTE 是每次消耗一个好还是直接消耗完呢? 现在是每次尽量消耗完,但有上限
	 */
	static void pairCallback(evutil_socket_t fd, short event, void *arg);

	/**
	 * 初始化线程通信管道
	 */
	int initSocketPair();

	AgentServer* getAgent(ClientIOSocket* socket);
	void deleteAgent(AgentServer* agent);

private:
	int _nPairDealCount;	//  每次最多处理多少线程数据

	event_base* _pBase;
	ClientSocketCallback* _pCB;

	std::vector<ClientSocketIOThread*> _vIOThreads;	// 线程列表
	int _nThreadCount;								// 线程数量
	int _nCurrentThread;							// 等待分配的游标

	Lock _lock;
	event* _pThreadEvent;
	evutil_socket_t _fdPair[2]; // 线程通信socket
	std::queue<threadPackage> _listPackList; // 线程通信队列

	std::map<unsigned long long, AgentServer*> _mapAgents;
};
