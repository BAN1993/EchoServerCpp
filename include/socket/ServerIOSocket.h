#pragma once

/*=============================================
=	监听的客户端Socket

 +	NOTE
		1.支持websocket
		2.本身不带锁，上层注意加锁

==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <socket/SocketBase.h>

class ServerSocketIOThread;

class ServerIOSocket : public unCopyable
{
	friend class ServerSocketIOThread;

public:
	ServerIOSocket(unsigned long long fd, ServerSocketIOThread* thread, bufferevent *bev, sockaddr_in addr);
	~ServerIOSocket();

public:
	inline unsigned long long getFd(){ return _nFd; }; // socketfd
	inline sockaddr_in getAddr() { return _addr; };
	inline ServerSocketIOThread* getThread() { return _pThread; };
	
private:
	/**
	 * 接收网络数据
	 * 会做网络类型判断
	 * 如果是websocket,会处理握手,去掉包头操作
	 */
	int recv();

	/**
	 * 关闭连接
	 * 真正关闭连接的地方,不允许逻辑层直接调用
	 */
	int toClose();

	/**
	 * 真正发送网络数据的地方
	 * 接口里面不再限制发送长度了，若超过 MAX_SEND_BUF_SIZE 也不管
	 */
	int toSend(char* buf, unsigned int len);

	/**
	 * 删除自己
	 */
	bool release();

	/**
	 * 向发送缓存尾部添加数据以减少网络操作次数
	 * 返回false说明缓存区已满，这时候需要消耗掉缓存
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
	 * 处理websocket协议,内容包括(类型判断,握手)
	 * return: 是否要放弃这些buf
	 */
	bool doWebSocket(char* buf, unsigned int len);

	/**
	 * 执行websocket握手
	 */
	bool doWebSocketHandShake(char* buf, unsigned int len);

	/**
	 * 去掉websocket协议头
	 */
	bool removeWebSocketHead(char* buf, unsigned int& len);

	/**
	 * 添加websocket包头并发送
	 */
	bool sendWebSocketBuff(char* buf, unsigned int& len);

private:
	unsigned int _status;			// 连接状态，见eIOSocketStatus
	unsigned long long _nFd;
	unsigned int _nWebSocketFlag;	// 是否为websocket, 见eWebSocketFlag
	ServerSocketIOThread* _pThread;	// 线程
	bufferevent *_pBev;
	sockaddr_in _addr;

	char* _pRecvBuf;				// 接收缓存
	unsigned int _nRecvBufLen;
	
	/**
	 * websocket保证协议完整度，且不会粘包，所以需要先把数据存到另一个地方去掉包头再放入缓存
	 * 这么做的原因是发送端可能会将多个或半个协议组成一个ws包，上层逻辑不一定会将缓存取完
	 */
	char* _tmpBuf;

	char* _pSendBuf;				// 发送缓存
	unsigned int _nSendBufLen;
};
