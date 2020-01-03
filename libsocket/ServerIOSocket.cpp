#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Log.h>
#include <base/sha1.h>
#include <base/base64.h>
#include <base/Utils.h>
#include <base/MallocManager.h>
#include <protocol/ProtocolBase.h>

#include <socket/ServerIOSocket.h>
#include <socket/ServerSocket.h>
#include <socket/ServerSocketIOThread.h>

ServerIOSocket::ServerIOSocket(unsigned long long fd, ServerSocketIOThread* thread, bufferevent *bev, sockaddr_in addr)
	: _pThread(thread), _pBev(bev)
{
	_status = eIOSocketStatus::unconnected;
	_nFd = fd;
	_nWebSocketFlag = eWebSocketFlag::wf_unknown;
	_addr = addr;

	_pRecvBuf = __new(MAX_BUFFER_SIZE);
	assert(_pRecvBuf);
	_nRecvBufLen = 0;
	_tmpBuf = nullptr;

	_pSendBuf = __new(MAX_BUFFER_SIZE);
	assert(_pSendBuf);
	_nSendBufLen = 0;
}

ServerIOSocket::~ServerIOSocket()
{
	__delete(_pRecvBuf);
	if(_tmpBuf)
		__delete(_tmpBuf);
	__delete(_pSendBuf);
}

int ServerIOSocket::recv()
{
	if(_status != eIOSocketStatus::connected)
	{
		LOGE("no connected,status="<<_status<<",fd="<<getFd());
		return -1;
	}
	unsigned int len = 0;
	if(_nWebSocketFlag == eWebSocketFlag::isWebSocket)
	{
		// websocket需要先去掉包头
		len = (unsigned int)bufferevent_read(_pBev, _tmpBuf, MAX_WS_TMP_BUFFER_SIZE);

		if(!removeWebSocketHead(_tmpBuf, len))
			return -2;

		memcpy(_pRecvBuf+_nRecvBufLen, _tmpBuf, len);
	}
	else
	{
		len = (unsigned int)bufferevent_read(_pBev,
			_pRecvBuf + _nRecvBufLen, 
			MAX_BUFFER_SIZE - _nRecvBufLen);

		if(!doWebSocket(_pRecvBuf, len))
			return -3;
	}

	// 解析出N个完整的包向上抛
	_nRecvBufLen += len;

	unsigned int tmpIndex = 0;
	while(true)
	{
		if(int(_nRecvBufLen - tmpIndex) <= 0)
			break;

		ProtocolHead head;
		try
		{
			bistream bis;
			bis.attach(_pRecvBuf, len);
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
		memcpy(buf, _pRecvBuf+tmpIndex, head.len);
		tmpIndex += head.len;

		getThread()->getServerSocket()->asynRecive(this, buf, head.len);
	}

	if(tmpIndex > 0)
	{
		memmove(_pRecvBuf, _pRecvBuf + tmpIndex, _nRecvBufLen - tmpIndex);
		_nRecvBufLen -= tmpIndex;
	}

	return 0;
}

int ServerIOSocket::toClose()
{
	_status = eIOSocketStatus::closed;
	if (_pBev != nullptr)
	{
		bufferevent_free(_pBev);
		_pBev = nullptr;
	}
	return 0;
}

int ServerIOSocket::toSend(char* buf, unsigned int len)
{
	if (_status != eIOSocketStatus::connected)
	{
		LOGE("Socket not connected,status=" << _status << ",fd=" << getFd());
		return -1;
	}

	if (_nWebSocketFlag == eWebSocketFlag::isWebSocket)
		return sendWebSocketBuff(buf, len);
	else
		return bufferevent_write(_pBev, buf, len);
}

bool ServerIOSocket::release()
{
	__delete_obj(ServerIOSocket, this);
	return true;
}

bool ServerIOSocket::expandSendBuf(char* buf, unsigned int len)
{
	unsigned wsLenAdd = (_nWebSocketFlag == eWebSocketFlag::isWebSocket) ? 10 : 0; 
	if((len+_nSendBufLen+wsLenAdd)>= MAX_BUFFER_SIZE)
	{
		// 缓存区已满，不能再添加
		//LOGE("send buf is full,nowlen=" << _nSendBufLen << ",len=" << len);
		return false;
	}
	
	memcpy(_pSendBuf+_nSendBufLen, buf, len);
	_nSendBufLen += len;

	return true;
}

bool ServerIOSocket::consumeSendBuf()
{
	if (!hadSendBufData()) return true;

	// 这里先不管包最大限制
	toSend(_pSendBuf, _nSendBufLen);
	_nSendBufLen = 0;
	return true;
}

bool ServerIOSocket::hadSendBufData()
{
	return _nSendBufLen > 0 ? true : false;
}

bool ServerIOSocket::doWebSocket(char* buf, unsigned int len)
{
	if (_nWebSocketFlag == eWebSocketFlag::notWebSocket
		|| _nWebSocketFlag == eWebSocketFlag::isWebSocket)
		return true;

	if (_nWebSocketFlag == eWebSocketFlag::wf_unknown)
	{
		if (strstr(buf, "GET") != nullptr)
		{
			LOG("recv web hand shake");
			if (doWebSocketHandShake(buf, len))
			{
				LOG("is websocket");
				_nWebSocketFlag = eWebSocketFlag::isWebSocket;
				_tmpBuf = __new(MAX_WS_TMP_BUFFER_SIZE);
				return false;
			}
		}
		_nWebSocketFlag = eWebSocketFlag::notWebSocket;
		return true;
	}
	return true;
}

bool ServerIOSocket::doWebSocketHandShake(char* buf, unsigned int len)
{
	// 请求key
	char reqKey[25] = { 0 };
	char* pos = strstr(buf, "Sec-WebSocket-Key");
	if (nullptr == pos)
		return false;
	memcpy(reqKey, pos + 19, 24);

	// 生成回复key
	char realKey[128] = { 0 };
	sprintf(realKey, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", reqKey);
	SHA1 sha;
	unsigned int message_digest[5];
	sha.Reset();
	sha << realKey;
	sha.Result(message_digest);
	for (int i = 0; i < 5; i++)
	{
		message_digest[i] = htonl(message_digest[i]);
	}
	char key[32] = { 0 };
	EncodeBase64(reinterpret_cast<const unsigned char*>(message_digest), (unsigned char*)key, 20);

	//拼接协议返回给客户端
	char response[1024] = { 0 };
	sprintf(response,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n\r\n", key);

	return toSend(response, (unsigned int)strlen(response)) == 0 ? true : false;
}

bool ServerIOSocket::removeWebSocketHead(char* buf, unsigned int& len)
{
	// 分片标识
	//unsigned int nFIN = ((buf[0] & 0x80) == 0x80) ? 1 : 0;

	// 数据类型
	unsigned int nOpcode = buf[0] & 0x0F;

	// mask必须为1
	//unsigned int nMask = ((buf[1] & 0x80) == 0x80) ? 1 : 0;

	// 数据长度,小于126就用一个字节表示数据长度
	len = buf[1] & 0x7F;

	// 几字节表示数据长度
	unsigned int playloadByteNum = 1;

	// mask key长度
	unsigned int maskKeyLen = 4;

	if (len == 126)
	{
		// 如果等于126，后续2字节表示长度
		playloadByteNum = 3;

		unsigned short tmpLen = 0;
		memcpy(&tmpLen, buf + 2, 2);
		len = ntohs(tmpLen);
	}
	else if (len == 127)
	{
		// 如果是127,后续8字节表示长度
		playloadByteNum = 9;

		unsigned int tmpLen = 0;
		memcpy(&tmpLen, buf + 2, 8);
		len = ntohl(tmpLen);
	}

	eWebSocketOpCode witch = eWebSocketOpCode(nOpcode);
	switch (witch)
	{
	case eWebSocketOpCode::continueFrame:
		LOGI("recv a continue frame");
		break;
	case eWebSocketOpCode::textFrame:
	{
		LOGI("recv a text frame");
		break;
	}
	case eWebSocketOpCode::binaryFrame:
	{
		LOGD("recv a binary,fd="<< getFd());
		char maskKey[4] = { 0 };
		memcpy(maskKey, buf + 1 + playloadByteNum, maskKeyLen);

		memmove(buf, buf + 1 + playloadByteNum + maskKeyLen, len);

		for (unsigned int i = 0; i < len; i++)
		{
			buf[i] = (char)(buf[i] ^ maskKey[i % maskKeyLen]);
		}
		buf[len] = '\0';
		return true;
	}
	case eWebSocketOpCode::closeFrame:
	{
		LOGI("recv a close protocol,fd=" << getFd());
		_status = eIOSocketStatus::while_Close;
		// TODO 现在会发生调用多次close的情况,试试这里不调用会怎么样
		//getThread()->getServerSocket()->asynClose(this);
		break;
	}
	case eWebSocketOpCode::pingFrame:
		LOGI("recv a ping frame");
		break;
	case eWebSocketOpCode::pongFrame:
		LOGI("recv a pong frame");
		break;
	default:
		break;
	}
	return false;
}

bool ServerIOSocket::sendWebSocketBuff(char* buf, unsigned int& len)
{
	int nBodyLenByteNum = 0;
	if (len >= 65536)
	{
		nBodyLenByteNum = 8;
	}
	else if (len >= 126)
	{
		nBodyLenByteNum = 2;
	}

	char* sendbuf = __new(len + 16);
	unsigned int weblen = 2;
	sendbuf[0] = 0;
	sendbuf[1] = 0;
	sendbuf[0] |= 0x80;

	// 确定是ws后就要开始发字节流了，因为发握手时还是未知状态
	if(_nWebSocketFlag == eWebSocketFlag::isWebSocket)
		sendbuf[0] |= eWebSocketOpCode::binaryFrame;
	else
		sendbuf[0] |= eWebSocketOpCode::textFrame;

	if (nBodyLenByteNum == 0)
	{
		sendbuf[1] = len;
	}
	else if (nBodyLenByteNum == 2)
	{
		sendbuf[1] = 126;
		short exlen = htons((unsigned short)len);
		memcpy(sendbuf+2, (const char*)(&exlen), 2);
		weblen = 4;
	}
	else
	{
		sendbuf[1] = 127;
		long exlen = htonl((unsigned long)len);
		memcpy(sendbuf + 2, (const char*)(&exlen), 8);
		weblen = 10;
	}
	
	memcpy(sendbuf + weblen, buf, len);
	len += weblen;

	if(!_pBev)
	{
		LOGE("bev is null,may close");
		return false;
	}

	bufferevent_write(_pBev, sendbuf, len);
	__delete(sendbuf);
	return true;
}
