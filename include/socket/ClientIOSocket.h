#pragma once

/*=============================================
=	主动连接的客户端Socket

+	NOTE
		1.本身不带锁，上层注意加锁

==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <socket/SocketBase.h>

class ClientSocketIOThread;

class ClientIOSocket : public unCopyable
{
	friend class ClientSocketIOThread;

public:
	ClientIOSocket(int id, ClientSocketIOThread* thread, sockaddr_in addr);
	~ClientIOSocket();

public:
	inline ClientSocketIOThread* getThread() { return _pThread; }
	inline int getID(){ return _nId; }
	inline unsigned long long getFd(){ return _nFd; }
	inline void setReConnectTime(int time) { _nReConnectTime = time; }
	
private:
	/**
	 * 真正建立连接的地方
	 */
	int toConnect();

	/**
	 * 尝试重连
	 */
	int toReConnect();

	/**
	 * 真正发送网络数据的地方
	 * 接口里面不再限制发送长度了，若超过 MAX_SEND_BUF_SIZE 也不管
	 */
	int toSend(char* buf, unsigned int len);

	/**
	 * 向发送缓存尾部添加数据以减少网络操作次数
	 * 返回false说明缓存区已满，这时候需要将数据添加到threadPackage里
	 */
	bool expandSendBuf(char* buf, unsigned int len);
	
	/**
	* 消耗掉发送缓存
	* 没数据 todo
	*/
	bool consumeSendBuf();

	/**
	* 发送缓存是否有数据 todo
	*/
	bool hadSendBufData();

	/**
	 * event回调,收到客户端数据
	 * 通知IOServerSocket收到数据
	 */
	static void readCallback(struct bufferevent *bev, void *arg);
	
	/**
	 * 处理收到的数据
	 * 由readCallback调用而来
	 */
	int recv();

	/**
	 * event回调,收到特殊事件,一般为连接超时或关闭
	 * 释放 bufferevent 并通知上层连接关闭
	 */
	static void eventCallback(struct bufferevent *bev, short events, void *arg);

	/**
	 * event回调,自动重连
	 */
	static void reconnectCallback(evutil_socket_t fd, short event, void *arg);

private:
	int _nId;
	unsigned int _nStatus;
	event_base* _pBase;
	ClientSocketIOThread* _pThread;
	unsigned long long _nFd;
	sockaddr_in _addr;
	bufferevent *_pBev;

	int _nReConnectTime;
	event* _pReConnEvent;

	char* _pRecvBuf; // 接收缓存
	unsigned int _nRecvBufLen;
	char* _pSendBuf; // 发送缓存
	unsigned int _nSendBufLen;
};
