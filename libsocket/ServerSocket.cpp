#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Log.h>
#include <base/MallocManager.h>
#include <protocol/ProtocolBase.h>

#include <socket/ServerSocket.h>
#include <socket/ServerSocketIOThread.h>
#include <socket/ServerIOSocket.h>
#include <socket/AgentClient.h>

ServerSocket::ServerSocket(int port, ServerSocketCallBack* cb, unsigned int threadcnt)
{
	_nListenSocketFd = -1;
	_nPort = port;
	_nIOSocketTimeOut = 10;
	_nMaxSockets = 5000;
	_nNowSocketCount = 0;
	_nPairDealCount = 300;
	_testFlag = 0;

	assert(initNet() == 0);

	_pCB = cb;

	_pBase = event_base_new();
	assert(_pBase);
	event_base_priority_init(_pBase, pAllCount);

	// timer最优先,尝试增加精度
	_pTimerEvent = event_new(_pBase, -1, EV_PERSIST, timerCallback, (void*)this);
	event_priority_set(_pTimerEvent, pTimer);
	
	// 帧计时器
	_pFrameEvent = event_new(_pBase, -1, EV_PERSIST, timerFrameCallback, (void*)this);
	event_priority_set(_pFrameEvent, pTimer);

	assert(initSocketPair() == 0);

	_nThreadCount = threadcnt;
	assert(_nThreadCount > 0);
	for (int i = 0; i < _nThreadCount; i++)
	{
		__new_obj_1(ServerSocketIOThread, t, this);
		_vIOThreads.push_back(t);
	}
	_nCurrentThread = 0;
}

ServerSocket::~ServerSocket()
{
	event_base_loopexit(_pBase, nullptr);
	if (_pListenEvent)
		event_free(_pListenEvent);
	event_free(_pTimerEvent);
	event_free(_pFrameEvent);
	event_free(_pThreadEvent);
	evutil_closesocket(_nListenSocketFd);

	if (_pBase)
		event_base_free(_pBase);

	for (int i = 0; i < _nThreadCount; ++i)
	{
		__delete(_vIOThreads[i]);
	}
}

int ServerSocket::begin()
{
	_nListenSocketFd = (int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_nListenSocketFd < 0)
		return -2;

	if (evutil_make_listen_socket_reuseable(_nListenSocketFd) < 0)
		return -3;
	if (evutil_make_socket_nonblocking(_nListenSocketFd) < 0)
		return -4;

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(_nPort);

	if (bind(_nListenSocketFd, (sockaddr*)&addr, sizeof(addr)) < 0)
		return -5;

	if (listen(_nListenSocketFd, 1024) < 0)
		return -6;

	_pListenEvent = event_new(_pBase, _nListenSocketFd, EV_READ | EV_PERSIST, acceptCallback, (void*)this);
	event_priority_set(_pListenEvent, pSocketListen);
	if (!_pListenEvent)
		return -7;

	if (event_add(_pListenEvent, nullptr) < 0)
		return -8;

	for (unsigned int i = 0; i < _vIOThreads.size(); ++i)
	{
		if (!_vIOThreads[i]->start())
			return -9;
	}
	
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	event_add(_pTimerEvent, &tv);

	event_base_dispatch(_pBase);
	LOGI("thread end");

	return 0;
}

int ServerSocket::registFrameTimer(unsigned int msec)
{
	timeval tv;
	tv.tv_sec = (msec / 1000);
	tv.tv_usec = (msec % 1000) * 1000;
	return event_add(_pFrameEvent, &tv);
}

int ServerSocket::asynAccept(ServerIOSocket* socket)
{
	threadPackage p;
	p.cmd = threadPackage::cmdtype::accept;
	p.name.socket = socket;

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

int ServerSocket::asynRecive(ServerIOSocket* socket, char* buf, unsigned int len)
{
	threadPackage p;
	p.cmd = threadPackage::cmdtype::recv_buf;
	p.name.socket = socket;
	p.buf = buf;
	p.len = len;

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

int ServerSocket::asynClose(ServerIOSocket* socket)
{
	threadPackage p;
	p.cmd = threadPackage::cmdtype::close;
	p.name.socket = socket;

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

void ServerSocket::acceptCallback(evutil_socket_t listensocket, short event, void *arg)
{
	ServerSocket *ss = static_cast<ServerSocket*>(arg);

	sockaddr addr;
	socklen_t addrlen = sizeof(addr);
	unsigned long long clientfd = accept(listensocket, &addr, &addrlen);
	
	if (clientfd < 0)
	{
		LOGE("accept socket fail:err=" << evutil_socket_error_to_string(errno) << ",errno=" << errno);
	}
	else
	{
		if (ss->_nNowSocketCount >= ss->_nMaxSockets)
		{
			// 超过最大连接限制,这里就不通知出去了
			LOGE("Reach the limit,fd=" << clientfd);
			evutil_closesocket(clientfd);
			return;
		}

		LOGD("accept a client,fd=" << clientfd);
		sockaddr_in* addr_in = (sockaddr_in*)&addr;

		ss->_vIOThreads[ss->_nCurrentThread]->asynAccept(clientfd, *addr_in);

		if (++ss->_nCurrentThread >= ss->_nThreadCount)
			ss->_nCurrentThread = 0;
	}
}

void ServerSocket::pairCallback(evutil_socket_t fd, short event, void *arg)
{
	ServerSocket *server = static_cast<ServerSocket*>(arg);

	char buf[16] = { 0 };
	recv(server->_fdPair[0], buf, 16, 0);

	unsigned long long begin = getTimeStampMsec();
	unsigned long long lockTime = 0;
	unsigned long long recvTime = 0;
	unsigned long long pairSendTime = 0;

	int dealCount = 0;
	while ((dealCount++) <= server->_nPairDealCount)
	{
		threadPackage p;
		{
			unsigned long long tmpBegin = getTimeStampMsec();
			if (server->_listPackList.empty())
				break;

			SelfLock l(server->_lock);
			p = server->_listPackList.front();
			server->_listPackList.pop();
			lockTime += (getTimeStampMsec() - tmpBegin);
		}

		if (p.cmd == threadPackage::cmdtype::accept)
		{
			ServerIOSocket* socket = (ServerIOSocket*)p.name.socket;
			if (socket)
			{
				AgentClient* agent = server->getAgent(socket);
				if (!agent)
				{
					server->_nNowSocketCount++;
					LOGD("accept a socket,fd=" << socket->getFd() << ",now count=" << server->_nNowSocketCount << ",scoket=" << socket);

					__new_obj_1(AgentClient, ac, socket);
					server->_mapAgents[socket->getFd()] = ac;

					server->getCB()->onAcceptClient(ac);
				}
				else
				{
					LOGE("already have fd:" << socket->getFd() << ",try add=" << socket << ",buf=" << agent->getSocket());
				}
			}
			else
				LOGE("socket is null");
		}
		else if (p.cmd == threadPackage::cmdtype::recv_buf)
		{
			unsigned long long tmpBegin = getTimeStampMsec(); // TTT
			ServerIOSocket* socket = (ServerIOSocket*)p.name.socket;
			if (socket)
			{
				AgentClient* agent = server->getAgent(socket);
				if (agent)
				{
					try
					{
						// 包头放到这里解析
						// 到这里说明是个完整包了,不用顾虑长度问题
						ProtocolHead head;
						bistream bis;
						bis.attach((char*)p.buf, p.len);
						bis >> head;

						server->getCB()->onClientRecive(agent, head, (char*)p.buf, p.len);
					}
					catch (biosexception& e)
					{
						// 解析协议失败
						LOGE("Protocol exception,cause=" << e.m_cause);
						agent->close();
					}
				}
				else
				{
					LOGE("can not get agent,fd=" << socket->getFd() << ",socket=" << socket);
					agent->close();
				}
			}
			else
				LOGE("socket is null");

			__delete(p.buf);
			recvTime += (getTimeStampMsec() - tmpBegin); // TTT
		}
		else if (p.cmd == threadPackage::cmdtype::close)
		{
			ServerIOSocket* socket = (ServerIOSocket*)p.name.socket;
			if (socket)
			{
				server->_nNowSocketCount--;
				LOGD("close a socket,fd=" << socket->getFd() << ",now count=" << server->_nNowSocketCount << ",socket=" << socket);

				AgentClient* agent = server->getAgent(socket);
				if(agent)
				{
					server->getCB()->onClientClose(agent);
					server->deleteAgent(agent);
				}
				else
				{
					LOGE("can not get agent,fd=" << socket->getFd() << ",socket=" << socket);
				}

				ServerSocketIOThread* thread = socket->getThread();
				thread->asynClose(socket);
			}
			else
				LOGE("socket is null");
		}
		else
		{
			LOGE("error cmd=" << p.cmd);
			if (GET_SINGLE(mallocManager)->avail(p.buf))
				__delete(p.buf);
		}
	}

	// 由于现在加了处理上限,很可能未处理完就没有任何新事件了
	// 所以如果队列还有事件,则再手动触发
	{
		unsigned long long tmpBegin = getTimeStampMsec();
		if (!server->_listPackList.empty())
			send(server->_fdPair[1], "1", 1, 0);
		pairSendTime = getTimeStampMsec() - tmpBegin;
	}

	unsigned long long use = getTimeStampMsec() - begin;
	if (use > 500)
	{
		// 处理时间超过500再打印
		LOGE("deal=" << dealCount << ",use=" << use << ",lock=" << lockTime << ",recv=" << recvTime
			<< ",pair=" << pairSendTime << ",nowsize=" << server->_listPackList.size());
	}
}

void ServerSocket::timerCallback(evutil_socket_t fd, short event, void *arg)
{
	ServerSocket *server = static_cast<ServerSocket*>(arg);
	server->getCB()->onTimer();
}

void ServerSocket::timerFrameCallback(evutil_socket_t fd, short event, void *arg)
{
	ServerSocket *server = static_cast<ServerSocket*>(arg);
	server->getCB()->onFrameTimer();
}

int ServerSocket::initSocketPair()
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

int ServerSocket::closeByLogic(AgentClient* agent)
{
	_nNowSocketCount--;
	LOGI("close a socket,fd=" << agent->getSocket()->getFd() << ",now count=" << _nNowSocketCount << ",socket=" << agent->getSocket());
	getCB()->onClientClose(agent);
	deleteAgent(agent);

	ServerSocketIOThread* thread = agent->getSocket()->getThread();
	return thread->asynClose(agent->getSocket());
}

AgentClient* ServerSocket::getAgent(ServerIOSocket* socket)
{
	if (!socket)
		return nullptr;

	std::map<unsigned long long, AgentClient*>::iterator it = _mapAgents.find(socket->getFd());
	if (it != _mapAgents.end())
		return it->second;
	return nullptr;
}

void ServerSocket::deleteAgent(AgentClient* agent)
{
	if (!agent) return;

	ServerIOSocket* socket = agent->getSocket();
	if (socket)
	{
		unsigned long long fd = socket->getFd();

		std::map<unsigned long long, AgentClient*>::iterator it = _mapAgents.find(fd);
		if (it != _mapAgents.end())
			_mapAgents.erase(it);
	}

	__delete_obj(AgentClient, agent);
}
