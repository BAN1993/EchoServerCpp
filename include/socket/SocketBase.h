#pragma once

extern "C"
{
#include <event2/event.h>
#include <event2/bufferevent.h>
}

#if defined _WINDOWS_ || defined WIN32

	#define socklen_t int

#else

	#include <sys/socket.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/syscall.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	
	#define INVALID_SOCKET -1

#endif

/**
* 每次发送的最大数据长度，也代表了包最大长度
* 注意websocket应该减去头部长度，一般当10
*/
const static unsigned int MAX_SEND_BUF_SIZE = 4096 - 10;
const static unsigned int MAX_BUFFER_SIZE = 8192;        // 收发缓存区大小
const static unsigned int MAX_WS_TMP_BUFFER_SIZE = 4096;  // websocket缓存大小

/**
* 每次pairCallback最多能处理的包
* 一次处理太多可能会阻塞timer或其他回调,所以这里限制下
* 放到配置里: pairdealcount
*/
//const static unsigned int MAX_PAIR_DEAL_COUNT = 300;

// websocket标识
enum eWebSocketFlag
{
	wf_unknown = 0,
	notWebSocket = 1,
	isWebSocket = 2,
};

// 数据包操作类型
enum eWebSocketOpCode
{
	continueFrame = 0x0,			//连续帧
	textFrame = 0x1,				//文本帧
	binaryFrame = 0x2,				//二进制帧
	closeFrame = 0x8,				//连接关闭
	pingFrame = 0x9,
	pongFrame = 0xA
};

enum eIOSocketStatus
{
	unconnected = 0,
	wait_connect,	// 等待连接成功
	connected,		// 已连接,准备好接包
	while_Close,	// 即将要关闭，还能继续发但是不能收
	closed,			// 已关闭
};

// event优先级
enum eEventPriority
{
	pTimer = 1,			// 计时器
	pSocketListen,		// 监听
	pSocketConnect,		// 请求连接
	pThreadPair,		// 线程通信
	pSocketWrite,		// 写消息
	pSocketRead,		// 读消息
	pAllCount,			// 优先级数目
};

// 线程间通信的结构体
struct threadPackage
{
	enum cmdtype
	{
		accept = 0,
		connect,
		connect_end,
		send_buf,
		recv_buf,
		close,
	};

	unsigned int cmd;

	union name
	{
		unsigned long long fd;
		void* socket; // IOServerSocket
	}name;

	unsigned int len;
	void* buf;

	threadPackage(){ memset(this, 0, sizeof(*this)); };
};
