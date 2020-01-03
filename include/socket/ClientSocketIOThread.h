#pragma once

/*=============================================
=	client Socket的io线程

==============================================*/

#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Thread.h>

class ClientIOSocket;
class ClientSocket;

class ClientSocketIOThread : public Thread
{
	DEFINE_UNCOPYABLE(ClientSocketIOThread);

	friend class ClientSocket;
	friend class ClientIOSocket;
	friend class AgentServer;

public:
	ClientSocketIOThread(ClientSocket* client, int recontime=5);
	~ClientSocketIOThread();

public:
	inline ClientSocket* getClientSocket() { return _pClientSocket; };

private:
	/**
	 * 开启线程循环
	 */
	int run();

	/**
	 * 主线程->io线程
	 * 主动连接
	 */
	int asynConnect(int id, std::string host, int port);
	
	/**
	 * 主线程->io线程
	 * 发送消息
	 * 会调用expandSendBuf来减少网络交互
	 * 若上面失败则还需要将数据加到threadPackage中
	 */
	int asynSend(ClientIOSocket* socket, char* buf, unsigned int len);

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

private:
	int _nReConnectTime;
	ClientSocket* _pClientSocket;
	event_base* _pBase;

	Lock _lock;
	event* _pThreadEvent;
	evutil_socket_t _fdPair[2]; // 线程通信socket
	std::queue<threadPackage> _listPackList;	// 主线程通知的数据
};
