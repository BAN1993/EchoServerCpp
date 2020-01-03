#pragma once

/*=============================================
 =	监听Socket的io线程

 +	NOTE
		这个模块对业务层应该是隐藏的

==============================================*/

#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Thread.h>

class ServerIOSocket;
class ServerSocket;

class ServerSocketIOThread : public Thread
{
	DEFINE_UNCOPYABLE(ServerSocketIOThread);

	friend class ServerSocket;
	friend class ServerIOSocket;
	friend class AgentClient;

public:
	ServerSocketIOThread(ServerSocket* server);
	~ServerSocketIOThread();

public:
	inline ServerSocket* getServerSocket() { return _pServerSocket; };

private:
	/**
	 * 开启线程循环
	 */
	int run();

	/**
	 * 主线程->io线程
	 * 将连接托管给子线程
	 * 设置好属性和回调函数后,再通知上层有新连接到来
	 */
	int asynAccept(unsigned long long fd, sockaddr_in& addr);

	/**
	 * 主线程->io线程
	 * 连接关闭,上层资源已被释放
	 */
	int asynClose(ServerIOSocket* socket);

	/**
	 * 主线程->io线程
	 * 发送消息
	 * 会调用expandSendBuf来减少网络交互
	 * 若上面失败则还需要将数据加到threadPackage中
	 * 
	 */
	int asynSend(ServerIOSocket* socket, char* buf, unsigned int len);

	/**
	 * event回调,收到线程数据
	 * 消耗 _listPackList
	 * NOTE 是每次消耗一个好还是直接消耗完呢? 现在是每次尽量消耗完,但有上限
	 */
	static void pairCallback(evutil_socket_t fd, short event, void *arg);

	/**
	 * event回调,收到客户端数据
	 * 通知IOServerSocket收到数据
	 */
	static void readCallback(struct bufferevent *bev, void *arg);

	/**
	 * event回调,收到特殊事件,一般为连接超时或关闭
	 * 释放 bufferevent 并通知上层连接关闭
	 */
	static void eventCallback(struct bufferevent *bev, short events, void *arg);

	/**
	* 初始化线程通信管道
	*/
	int initSocketPair();
	
private:
	ServerSocket* _pServerSocket;
	event_base* _pBase;

	Lock _lock;
	event* _pThreadEvent;
	evutil_socket_t _fdPair[2]; // 线程通信socket
	std::queue<threadPackage> _listPackList;	// 主线程通知的数据
};
