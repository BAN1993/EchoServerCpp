#include <base/Base.h>
#include <socket/SocketBase.h>
#include <base/Log.h>
#include <base/MallocManager.h>
#include <protocol/ProtocolBase.h>

#include <socket/ClientSocket.h>
#include <socket/ClientSocketIOThread.h>
#include <socket/ClientIOSocket.h>
#include <socket/AgentServer.h>

ClientSocket::ClientSocket(event_base* base,  ClientSocketCallback* cb, unsigned int threadcnt)
{
	_pBase = base;
	assert(_pBase);

	_pCB = cb;

	assert(initSocketPair() == 0);

	_nThreadCount = threadcnt;
	assert(_nThreadCount > 0);
	for (int i = 0; i < _nThreadCount; i++)
	{
		__new_obj_1(ClientSocketIOThread, t, this);
		_vIOThreads.push_back(t);
	}
	_nCurrentThread = 0;
}

ClientSocket::~ClientSocket()
{
	event_free(_pThreadEvent);
	evutil_closesocket(_fdPair[0]);
	evutil_closesocket(_fdPair[1]);
	
	for (int i = 0; i < _nThreadCount; ++i)
	{
		__delete(_vIOThreads[i]);
	}
}

int ClientSocket::begin()
{
	for (unsigned int i = 0; i < _vIOThreads.size(); ++i)
	{
		if (!_vIOThreads[i]->start())
			return -1;
	}

	return 0;
}

int ClientSocket::connect(int id, std::string host, int port)
{
	return asynConnect(id, host, port);
}

int ClientSocket::asynConnect(int id, std::string host, int port)
{
	int ret = _vIOThreads[_nCurrentThread]->asynConnect(id, host, port);
	if (ret != 1)
	{
		LOGE("asynConnect error");
	}
	else
		if (++_nCurrentThread >= _nThreadCount)
			_nCurrentThread = 0;
	return ret;
}

int ClientSocket::asynConnectEnd(ClientIOSocket* socket, int flag)
{
	char* i = __new(sizeof(int));
	*(int*)i = flag;

	threadPackage p;
	p.cmd = threadPackage::cmdtype::connect_end;
	p.name.socket = socket;
	p.buf = i;

	{
		SelfLock l(_lock);
		_listPackList.push(p);
	}
	return send(this->_fdPair[1], "1", 1, 0);
}

int ClientSocket::asynRecive(ClientIOSocket* socket, char* buf, unsigned int len)
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

int ClientSocket::asynClose(ClientIOSocket* socket)
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

void ClientSocket::pairCallback(evutil_socket_t fd, short event, void *arg)
{
	ClientSocket *clientSocket = static_cast<ClientSocket*>(arg);

	char buf[16] = { 0 };
	recv(clientSocket->_fdPair[0], buf, 16, 0);

	int dealCount = 0;
	while ((dealCount++) <= clientSocket->_nPairDealCount)
	{
		threadPackage p;
		{
			if (clientSocket->_listPackList.empty())
				break;

			SelfLock l(clientSocket->_lock);
			p = clientSocket->_listPackList.front();
			clientSocket->_listPackList.pop();
		}
		
		if (p.cmd == threadPackage::cmdtype::connect_end)
		{
			int flag = (int)*(int*)p.buf;
			__delete(p.buf);

			ClientIOSocket* socket = (ClientIOSocket*)p.name.socket;
			if (socket)
			{
				AgentServer* agent = clientSocket->getAgent(socket);
				if (!agent)
				{
					__new_obj_1(AgentServer, as, socket);
					clientSocket->_mapAgents[socket->getFd()] = as;

					clientSocket->getCB()->onConnectServer(as, flag);
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
			ClientIOSocket* socket = (ClientIOSocket*)p.name.socket;
			if (socket)
			{
				AgentServer* agent = clientSocket->getAgent(socket);
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

						clientSocket->getCB()->onServerRecive(agent, head, (char*)p.buf, p.len);
					}
					catch (biosexception& e)
					{
						// 解析协议失败
						// NOTE 这属于严重错误了,另个服务发来的数据不应该出现包头解析失败的情况
						LOGE("Protocol exception,cause=" << e.m_cause);
					}
				}
				else
				{
					// NOTE 这属于严重错误了,另个服务的连接不应该出现不存在agent的情况
					LOGE("can not get agent,fd=" << socket->getFd() << ",socket=" << socket);
				}
			}
			else
				LOGE("socket is null");

			__delete(p.buf);
		}
		else if (p.cmd == threadPackage::cmdtype::close)
		{
			ClientIOSocket* socket = (ClientIOSocket*)p.name.socket;
			if (socket)
			{
				AgentServer* agent = clientSocket->getAgent(socket);
				if (agent)
				{
					clientSocket->getCB()->onServerClose(agent);
					clientSocket->deleteAgent(agent);
				}
				else
				{
					// TODO 这里要不要close这个socket
					LOGE("can not get agent,fd=" << socket->getFd() << ",socket=" << socket);
				}
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
	if(!clientSocket->_listPackList.empty())
		send(clientSocket->_fdPair[1], "1", 1, 0);
}

int ClientSocket::initSocketPair()
{
#if defined _WINDOWS_ || defined WIN32
	int r = evutil_socketpair(AF_INET, SOCK_STREAM, 0, _fdPair);
#else
	int r = evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, _fdPair);
#endif

	if (r != 0)
		return r;

	if ((-1 == evutil_make_socket_nonblocking(_fdPair[0]))|| (-1 == evutil_make_socket_nonblocking(_fdPair[1])))
		return -1;

	_pThreadEvent = event_new(_pBase, _fdPair[0], EV_READ | EV_PERSIST, pairCallback, (void*)this);
	event_priority_set(_pThreadEvent, pThreadPair);
	if (!_pThreadEvent)
		return -1;

	return event_add(_pThreadEvent, nullptr);
}

AgentServer* ClientSocket::getAgent(ClientIOSocket* socket)
{
	if (!socket)
		return nullptr;

	std::map<unsigned long long, AgentServer*>::iterator it = _mapAgents.find(socket->getFd());
	if (it != _mapAgents.end())
		return it->second;
	return nullptr;
}

void ClientSocket::deleteAgent(AgentServer* agent)
{
	if (!agent) return;

	ClientIOSocket* socket = agent->getSocket();
	if (socket)
	{
		unsigned long long fd = socket->getFd();

		std::map<unsigned long long, AgentServer*>::iterator it = _mapAgents.find(fd);
		if (it != _mapAgents.end())
			_mapAgents.erase(it);
	}

	__delete_obj(AgentServer, agent);
}
