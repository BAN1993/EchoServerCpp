#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Log.h>
#include <base/MallocManager.h>

#include <socket/ClientSocketIOThread.h>
#include <socket/ClientSocket.h>
#include <socket/ClientIOSocket.h>

ClientSocketIOThread::ClientSocketIOThread(ClientSocket* client, int recontime)
	: Thread("ClientSocketIOThread"), _pClientSocket(client)
{
	_nReConnectTime = recontime;

	_pBase = event_base_new();
	assert(_pBase);

	event_base_priority_init(_pBase, pAllCount);

	assert(initSocketPair() == 0);
}

ClientSocketIOThread::~ClientSocketIOThread()
{
	event_base_loopexit(_pBase, nullptr);
	event_free(_pThreadEvent);

	if (_pBase)
		event_base_free(_pBase);

	evutil_closesocket(_fdPair[0]);
	evutil_closesocket(_fdPair[1]);
}

int ClientSocketIOThread::run()
{
	event_base_dispatch(_pBase);
	LOGI("thread end");
	return 0;
}

int ClientSocketIOThread::asynConnect(int id, std::string host, int port)
{
	__new_obj(sockaddr_in, adp);
	adp->sin_family = AF_INET;
	adp->sin_addr.s_addr = inet_addr(host.c_str());
	adp->sin_port = htons(port);

	threadPackage p;
	p.cmd = threadPackage::cmdtype::connect;
	p.name.fd = id;
	p.buf = adp;
	p.len = sizeof(sockaddr_in);

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

int ClientSocketIOThread::asynSend(ClientIOSocket* socket, char* buf, unsigned int len)
{
	//LOGD("fd=" << socket->getID() << ",len=" << len);

	// NOTE 这里把数据放到一个sendbuf里，这样能减少write次数，不知道能不能提升效率
	// NOTE 这样可能导致发送数据先后无法保证
	// 可以这样处理:这里就往队列塞,取的时候做缓存
	threadPackage p;
	p.cmd = threadPackage::cmdtype::send_buf;
	p.name.socket = socket;

	{
		SelfLock l(_lock);
		//if (socket->expandSendBuf(buf, len))
		//{
			// 成功了就delete掉缓存
		//	__delete(buf);
		//	p.buf = nullptr;
		//	p.len = 0;
		//}
		//else
		//{
			// 失败了就用队列保存
			p.buf = buf;
			p.len = len;
		//}
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

void ClientSocketIOThread::pairCallback(evutil_socket_t fd, short event, void *arg)
{
	ClientSocketIOThread *thread = static_cast<ClientSocketIOThread*>(arg);

	{
		char buf[16] = { 0 };
		recv(thread->_fdPair[0], buf, 16, 0);
	}

	std::list<ClientIOSocket*> needSendList;

	int dealCount = 0;
	while ((dealCount++) <= thread->getClientSocket()->_nPairDealCount)
	{
		threadPackage p;
		{
			SelfLock l(thread->_lock);
			if (thread->_listPackList.empty())
				break;

			p = thread->_listPackList.front();
			thread->_listPackList.pop();
		}

		if (p.cmd == threadPackage::cmdtype::connect)
		{
			sockaddr_in addr;
			memcpy(&addr, p.buf, sizeof(sockaddr_in));
			__delete_obj(sockaddr_in, (sockaddr_in*)p.buf);

			__new_obj_3(ClientIOSocket, socket, (int)p.name.fd, thread, addr);

			socket->setReConnectTime(thread->_nReConnectTime);
			socket->toConnect();
		}
		else if (p.cmd == threadPackage::cmdtype::send_buf)
		{
			ClientIOSocket* socket = (ClientIOSocket*)p.name.socket;
			if (socket)
			{
				if (!socket->expandSendBuf((char*)p.buf, p.len))
				{
					socket->consumeSendBuf();
					if (!socket->expandSendBuf((char*)p.buf, p.len))
						LOGE("WHY?????????,len="<<p.len);
				}
				else
					needSendList.push_back(socket);
			}
			else
				LOGE("socket is null");

			__delete(p.buf);
		}
		else
		{
			LOGE("error cmd=" << p.cmd);
			if (GET_SINGLE(mallocManager)->avail(p.buf))
				__delete(p.buf);
		}
	}

	// 消耗掉所有socket的发送缓存
	std::list<ClientIOSocket*>::iterator it = needSendList.begin();
	for (; it != needSendList.end(); it++)
	{
		if (!*it) continue;
		if ((*it)->hadSendBufData())
			(*it)->consumeSendBuf();
	}
	
	// 由于现在加了处理上限,很可能未处理完就没有任何新事件了
	// 所以如果队列还有事件,则再手动触发
	if(!thread->_listPackList.empty())
		send(thread->_fdPair[1], "1", 1, 0);
}

int ClientSocketIOThread::initSocketPair()
{
#if defined _WINDOWS_ || defined WIN32
	int r = evutil_socketpair(AF_INET, SOCK_STREAM, 0, _fdPair);
#else
	int r = evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, _fdPair);
#endif

	if (r != 0)
		return r;

	if ((-1 == evutil_make_socket_nonblocking(_fdPair[0])) || (-1 == evutil_make_socket_nonblocking(_fdPair[1])))
		return -1;

	_pThreadEvent = event_new(_pBase, _fdPair[0], EV_READ | EV_PERSIST, pairCallback, (void*)this);
	event_priority_set(_pThreadEvent, pThreadPair);
	if (!_pThreadEvent)
		return -1;

	return event_add(_pThreadEvent, nullptr);
}
