#include <base/Utils.h>
#include <base/Log.h>
#include <base/SystemMonitor.h>

#include "Server.h"

Server::Server()
{
	__new_obj_3(
		ServerSocket,
		s,
		GET_SINGLE(ServerConfig)->getListenPort(),
		this,
		GET_SINGLE(ServerConfig)->getServerThreads());

	_pServerSocket = s;
	assert(_pServerSocket);

	_pServerSocket->setMaxSocket(GET_SINGLE(ServerConfig)->getMaxSockets());
	_pServerSocket->setPairDealCount(GET_SINGLE(ServerConfig)->getPairDealCount());
	_pServerSocket->setTimeOut(GET_SINGLE(ServerConfig)->getServerTimeOut());

	_nSocketCount = 0;
	_nAcceptCount = 0;
	_nCloseCount = 0;
	_nReciveCount = 0;
	_nSendCount = 0;
};

Server::~Server()
{
	__delete_obj(ServerSocket, _pServerSocket);
}

int Server::begin()
{
	// 打印一些系统信息
	GET_SINGLE(SystemMonitor)->printAllInfo();

	return _pServerSocket->begin();
}

void Server::onAcceptClient(AgentClient* agent)
{
	//sockaddr_in addr = agent->getAddr();
	//LOGI("accept a client,ip=" << inet_ntoa(addr.sin_addr) << ",port=" << ntohs(addr.sin_port));
	
	_nAcceptCount++;
	_nSocketCount++;
}

void Server::onClientRecive(AgentClient* agent, ProtocolHead head, char* buf, unsigned int len)
{
	_nReciveCount++;

	if (head.xyid != PROTOCOL_XYID::XYID_HEARTBEAT)
	{
		LOGE("Head error,xyid=" << head.xyid << ",len=" << len);
		return;
	}

	try
	{
		Heartbeat req;
		READXY(req, buf, len);

		Heartbeat res;
		res.timestamp = (unsigned int)time(NULL);
		char *sendbuf = __new(sizeof(res));
		GET_TEMPLATE_BUF(sendbuf, head, res);

		agent->send(sendbuf, head.len);
		_nSendCount++;
	}
	catch (biosexception& e)
	{
		// 解析协议失败
		LOGE("Protocol exception,cause=" << e.m_cause << ",[[" << memToString(buf, len) << "]]");
	}
}

void Server::onClientClose(AgentClient* agent)
{
	//sockaddr_in addr = agent->getAddr();
	//LOGI("close a client,ip=" << inet_ntoa(addr.sin_addr) << ",port=" << ntohs(addr.sin_port));

	_nCloseCount++;
	_nSocketCount--;
}

void Server::onTimer()
{
	LOGI("sockets=" << _nSocketCount << ",accept=" << _nAcceptCount << ",close=" << _nCloseCount
		<< ",recv=" << _nReciveCount << ",send=" << _nSendCount);

	_nAcceptCount = 0;
	_nCloseCount = 0;
	_nReciveCount = 0;
	_nSendCount = 0;
}
