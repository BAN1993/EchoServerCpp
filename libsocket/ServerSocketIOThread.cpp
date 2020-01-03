#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Log.h>
#include <base/MallocManager.h>

#include <socket/ServerSocketIOThread.h>
#include <socket/ServerSocket.h>
#include <socket/ServerIOSocket.h>

ServerSocketIOThread::ServerSocketIOThread(ServerSocket* server)
	: Thread("ServerSocketIOThread"), _pServerSocket(server)
{
	_pBase = event_base_new();
	assert(_pBase);

	event_base_priority_init(_pBase, pAllCount);

	assert(initSocketPair() == 0);
}

ServerSocketIOThread::~ServerSocketIOThread()
{
	event_base_loopexit(_pBase, nullptr);
	event_free(_pThreadEvent);
	evutil_closesocket(_fdPair[0]);
	evutil_closesocket(_fdPair[1]);

	if (_pBase)
		event_base_free(_pBase);
}

int ServerSocketIOThread::run()
{
	event_base_dispatch(_pBase);
	LOGI("thread end");

	return 0;
}

int ServerSocketIOThread::asynAccept(unsigned long long fd, sockaddr_in& addr)
{
	LOGD("fd=" << fd << ",ip=" << inet_ntoa(addr.sin_addr) << ",port=" << ntohs(addr.sin_port));

	threadPackage p;
	p.cmd = threadPackage::cmdtype::accept;
	p.name.fd = fd;
	__new_obj(sockaddr_in, adp);
	memcpy(adp, &addr, sizeof(sockaddr_in));
	p.buf = adp;
	p.len = sizeof(sockaddr_in);

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

int ServerSocketIOThread::asynClose(ServerIOSocket* socket)
{
	LOGD("fd=" << socket->getFd() << ",ip=" << inet_ntoa(socket->_addr.sin_addr) << ",port=" << ntohs(socket->_addr.sin_port));

	threadPackage p;
	p.cmd = threadPackage::cmdtype::close;
	p.name.socket = socket;

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

int ServerSocketIOThread::asynSend(ServerIOSocket* socket, char* buf, unsigned int len)
{
	{
		SelfLock l(_lock);

		threadPackage p;
		p.cmd = threadPackage::cmdtype::send_buf;
		p.name.socket = socket;
		p.buf = buf;
		p.len = len;

		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

void ServerSocketIOThread::pairCallback(evutil_socket_t fd, short event, void *arg)
{
	ServerSocketIOThread *thread = static_cast<ServerSocketIOThread*>(arg);

	{
		char buf[16] = { 0 };
		recv(thread->_fdPair[0], buf, 16, 0);
	}

	std::list<ServerIOSocket*> needSendList;

	int dealCount = 0;
	while ((dealCount++) <= thread->getServerSocket()->_nPairDealCount)
	{
		threadPackage p;
		{
			SelfLock l(thread->_lock);
			if(thread->_listPackList.empty())
				break;

			p = thread->_listPackList.front();
			thread->_listPackList.pop();
		}

		if(p.cmd == threadPackage::cmdtype::accept)
		{
			evutil_make_socket_nonblocking(p.name.fd);

			bufferevent *bev = bufferevent_socket_new(thread->_pBase, p.name.fd, BEV_OPT_CLOSE_ON_FREE);
			if (!bev)
			{
				LOGE("new bev error," << evutil_socket_error_to_string(errno));
				__delete_obj(sockaddr_in, (sockaddr_in*)p.buf);
				continue;
			}

			sockaddr_in addr;
			memcpy(&addr, p.buf, sizeof(sockaddr_in));
			__delete_obj(sockaddr_in, (sockaddr_in*)p.buf);

			__new_obj_4(ServerIOSocket, s, p.name.fd, thread, bev, addr);
			LOGD("accept a socket,fd=" << p.name.fd);

			ServerSocket* server = thread->getServerSocket();
			if (server->_nIOSocketTimeOut > 0)
			{
				timeval tv;
				tv.tv_sec = server->_nIOSocketTimeOut;
				tv.tv_usec = 0;
				bufferevent_set_timeouts(bev, &tv, nullptr);
			}

			bufferevent_setcb(bev, readCallback, nullptr, eventCallback, s);
			if (bufferevent_enable(bev, EV_READ | EV_PERSIST) < 0)
				LOGE("bufferevent_enable error," << evutil_socket_error_to_string(errno));

			server->asynAccept(s);
			s->_status = eIOSocketStatus::connected;
		}
		else if(p.cmd == threadPackage::cmdtype::send_buf)
		{
			ServerIOSocket* socket = (ServerIOSocket*)p.name.socket;
			if(socket)
			{
				if (!socket->expandSendBuf((char*)p.buf, p.len))
				{
					socket->consumeSendBuf();
					if (!socket->expandSendBuf((char*)p.buf, p.len))
						LOGE("WHY?????????,len=" << p.len);
				}
				else
					needSendList.push_back(socket);
			}
			else
				LOGE("socket is null");

			__delete(p.buf);
		}
		else if(p.cmd == threadPackage::cmdtype::close)
		{
			ServerIOSocket* socket = (ServerIOSocket*)p.name.socket;
			if(socket)
			{
				socket->toClose();
				__delete_obj(ServerIOSocket, socket);
				socket = nullptr;
			}
			else
				LOGE("socket is null");

			//  如果内存暴涨，这里打印下哪里没释放
			//GET_SINGLE(mallocManager)->printMemRecode();
		}
		else
		{
			LOGE("error cmd="<<p.cmd);
			if (GET_SINGLE(mallocManager)->avail(p.buf))
				__delete(p.buf);
		}
	}
	
	// 消耗掉所有socket的发送缓存
	std::list<ServerIOSocket*>::iterator it = needSendList.begin();
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

void ServerSocketIOThread::readCallback(struct bufferevent *bev, void *arg)
{
	ServerIOSocket *socket = static_cast<ServerIOSocket*>(arg);
	socket->recv();
}

void ServerSocketIOThread::eventCallback(struct bufferevent *bev, short events, void *arg)
{
	ServerIOSocket* socket = static_cast<ServerIOSocket*>(arg);
	if (nullptr == socket)
	{
		LOGE("socket is null");
		bufferevent_free(bev);
		return;
	}

	ServerSocketIOThread* thread = socket->getThread();
	if (nullptr == thread)
	{
		LOGE("fd=" << socket->getFd() << ",thread is null");
		bufferevent_free(bev);
		__delete(socket);
		return;
	}

	if (events & BEV_EVENT_EOF)
	{
		LOGD("close:ip=" << inet_ntoa(socket->_addr.sin_addr)
			<< ",port=" << ntohs(socket->_addr.sin_port)
			<< ",fd=" << socket->getFd());
	}
	else if (events & BEV_EVENT_ERROR)
	{
		LOGD("fail:err=" << evutil_socket_error_to_string(errno)
			<< ",ip=" << inet_ntoa(socket->_addr.sin_addr)
			<< ",port=" << ntohs(socket->_addr.sin_port)
			<< ",fd=" << socket->getFd());
	}
	else if (events & BEV_EVENT_TIMEOUT)
	{
		LOGD("timeout:event=" << events
			<< ",err=" << evutil_socket_error_to_string(errno)
			<< ",ip=" << inet_ntoa(socket->_addr.sin_addr)
			<< ",port=" << ntohs(socket->_addr.sin_port)
			<< "fd=" << socket->getFd());
	}
	else
	{
		LOGD("unkown:event=" << events
			<< ",ip=" << inet_ntoa(socket->_addr.sin_addr)
			<< ",port=" << ntohs(socket->_addr.sin_port)
			<< ",fd=" << socket->getFd());
	}

	thread->getServerSocket()->asynClose(socket);
}

int ServerSocketIOThread::initSocketPair()
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
