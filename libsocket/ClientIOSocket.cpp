#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Log.h>
#include <base/Utils.h>
#include <base/MallocManager.h>
#include <protocol/ProtocolBase.h>

#include <socket/ClientIOSocket.h>
#include <socket/ClientSocket.h>
#include <socket/ClientSocketIOThread.h>

ClientIOSocket::ClientIOSocket(int id, ClientSocketIOThread* thread, sockaddr_in addr)
	: _pBase(thread->_pBase), _pThread(thread)
{
	_nId = id;
	_nStatus = eIOSocketStatus::unconnected;
	_nFd = -1;
	_addr = addr;

	assert(_pBase);

	_nReConnectTime = 5;
	_pReConnEvent = event_new(_pBase, -1, 0, reconnectCallback, (void*)this);
	event_priority_set(_pReConnEvent, pSocketConnect);

	_pRecvBuf = __new(MAX_BUFFER_SIZE);
	assert(_pRecvBuf);
	_nRecvBufLen = 0;

	_pSendBuf = __new(MAX_BUFFER_SIZE);
	assert(_pSendBuf);
	_nSendBufLen = 0;
}

ClientIOSocket::~ClientIOSocket()
{
	event_free(_pReConnEvent);
	__delete(_pRecvBuf);
	__delete(_pSendBuf);
}

int ClientIOSocket::toConnect()
{
	// NOTE 不知道为什么用bufferevent_socket_connect会一下触发两次event
	//		一次成功一次失败(对端未开启)
	//		所以改成用原生接口

	_nStatus = eIOSocketStatus::wait_connect;

	_nFd = socket(AF_INET, SOCK_STREAM, 0);
	if(_nFd <= 0)
	{
		LOGE("new socket error,err=" << strerror(errno));
		return -1;
	}

	int stat = ::connect(_nFd, (struct sockaddr *)&_addr, sizeof(_addr));
	if(stat == -1)
	{
		LOGE("connect to " << getID() << ":" << inet_ntoa(_addr.sin_addr) << ":" << ntohs(_addr.sin_port) << " faild");
		getThread()->getClientSocket()->asynClose(this);
		toReConnect();
		return -1;
	}

	stat = evutil_make_socket_nonblocking(_nFd);
	if(stat == -1)
	{
		LOGE("set nonblocking error,err=" << strerror(errno));
		getThread()->getClientSocket()->asynClose(this);
		toReConnect();
		return -1;
	}

	_pBev = bufferevent_socket_new(_pBase, _nFd, BEV_OPT_CLOSE_ON_FREE);
	if (!_pBev)
	{
		LOGE("bufferevent_socket_new fail:err=" << evutil_socket_error_to_string(errno));
		return -1;
	}

	bufferevent_setcb(_pBev, readCallback, nullptr, eventCallback, this);

	if (bufferevent_enable(_pBev, EV_READ | EV_WRITE | EV_PERSIST) < 0)
	{
		LOGE("bufferevent_enable fail:err=" << evutil_socket_error_to_string(errno));
		return -1;
	}

	//int fd = bufferevent_socket_connect(_pBev, (struct sockaddr *)&_addr, sizeof(_addr));
	//if (fd < 0)
	//{
	//	LOGE("bufferevent_socket_connect fail:err=" << evutil_socket_error_to_string(errno));
	//	return -1;
	//}
	//getThread()->getClientSocket()->getCB()->onConnectServer(this, 0);
	getThread()->getClientSocket()->asynConnectEnd(this, 0);

	return 0;
}

int ClientIOSocket::toReConnect()
{
	timeval tv;
	tv.tv_sec = _nReConnectTime;
	tv.tv_usec = 0;
	return event_add(_pReConnEvent, &tv);
}

int ClientIOSocket::toSend(char* buf, unsigned int len)
{
	return bufferevent_write(_pBev, buf, len);
}

bool ClientIOSocket::expandSendBuf(char* buf, unsigned int len)
{
	if ((len + _nSendBufLen) >= MAX_BUFFER_SIZE)
	{
		// 缓存区已满，不能再添加
		LOGE("send buf is full,nowlen=" << _nSendBufLen << ",len=" << len);
		return false;
	}

	memcpy(_pSendBuf + _nSendBufLen, buf, len);
	_nSendBufLen += len;
	return true;
}

bool ClientIOSocket::consumeSendBuf()
{
	if (!hadSendBufData()) return true;

	// 这里先不管包最大限制
	toSend(_pSendBuf, _nSendBufLen);
	_nSendBufLen = 0;
	return true;
}

bool ClientIOSocket::hadSendBufData()
{
	return _nSendBufLen > 0 ? true : false;
}

void ClientIOSocket::readCallback(struct bufferevent *bev, void *arg)
{
	ClientIOSocket *socket = static_cast<ClientIOSocket*>(arg);
	socket->recv();
}

int ClientIOSocket::recv()
{
	unsigned int len = (unsigned int)bufferevent_read(
		_pBev,
		_pRecvBuf + _nRecvBufLen,
		MAX_BUFFER_SIZE - _nRecvBufLen);

	// 解析出N个完整的包向上抛
	_nRecvBufLen += len;

	unsigned int tmpIndex = 0;
	while (true)
	{
		if (int(_nRecvBufLen - tmpIndex) < HEADLEN)
			break;

		//ProtocolHead* head = (ProtocolHead*)_pRecvBuf;
		ProtocolHead head;
		try
		{
			bistream bis;
			bis.attach(_pRecvBuf + tmpIndex, HEADLEN);
			bis >> head;

			if (head.len > int(_nRecvBufLen - tmpIndex))
				break;
		}
		catch (biosexception& e)
		{
			// 解析协议失败
			LOGE("Protocol exception,cause=" << e.m_cause);
			break;
		}

		char* buf = __new(head.len);
		memcpy(buf, _pRecvBuf + tmpIndex, head.len);
		tmpIndex += head.len;

		getThread()->getClientSocket()->asynRecive(this, buf, head.len);
	}

	if (tmpIndex > 0)
	{
		memmove(_pRecvBuf, _pRecvBuf + tmpIndex, _nRecvBufLen - tmpIndex);
		_nRecvBufLen -= tmpIndex;
	}

	return 0;
}

void ClientIOSocket::eventCallback(struct bufferevent *bev, short events, void *arg)
{
	ClientIOSocket* socket = static_cast<ClientIOSocket*>(arg);
	if (!socket) return;

	// 由于现在使用原生connect,成功失败不再已回调形式通知上来
	//if (events & BEV_EVENT_CONNECTED)
	//{
	//	LOGI("connect to id="<<socket->_nId<<" success,event=" << events);
	//
	//	socket->_nStatus = eIOSocketStatus::connected;
	//
	//	socket->getThread()->getClientSocket()->asynConnectEnd(socket, 0);
	//}
	if (events & BEV_EVENT_TIMEOUT)
	{
		LOGI("connect to id=" << socket->_nId << " time out,event=" << events);

		socket->_nStatus = eIOSocketStatus::wait_connect;

		socket->getThread()->getClientSocket()->asynClose(socket);
		socket->toReConnect();
	}
	else if (events & BEV_EVENT_ERROR)
	{
		LOGE("connect fail id=" << socket->_nId << ",event=" << events
			<< ",err=" << evutil_socket_error_to_string(errno) << ",errno=" << errno);

		socket->_nStatus = eIOSocketStatus::wait_connect;

		socket->getThread()->getClientSocket()->asynClose(socket);
		socket->toReConnect();
	}
	else if (events & BEV_EVENT_EOF)
	{
		LOGE("connect close id=" << socket->_nId << ",event=" << events);

		socket->_nStatus = eIOSocketStatus::wait_connect;

		socket->getThread()->getClientSocket()->asynClose(socket);
		socket->toReConnect();
	}
	else
	{
		LOGE("unknown event=" << events);
	}
}

void ClientIOSocket::reconnectCallback(evutil_socket_t fd, short event, void *arg)
{
	ClientIOSocket* socket = static_cast<ClientIOSocket*>(arg);
	socket->toConnect();
}
