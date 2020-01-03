#pragma once

/*=============================================
 =	服务类

==============================================*/

#include <base/Base.h>
#include <socket/SocketBase.h>
#include <tool/Uncopyable.h>
#include <base/MallocManager.h>
#include <socket/SocketBase.h>
#include <socket/ServerSocket.h>
#include <socket/AgentClient.h>
#include <protocol/ProtocolBase.h>

#include "ServerConfig.h"
#include "SomeDefine.h"

class Server :
				public unCopyable,
				public ServerSocketCallBack
{
public:
	Server();
	~Server();

public:
	int begin();

public:
	virtual void onAcceptClient(AgentClient* agent);
	virtual void onClientRecive(AgentClient* agent, struct ProtocolHead head, char* buf, unsigned int len);
	virtual void onClientClose(AgentClient* agent);
	virtual void onTimer();
	virtual void onFrameTimer(){};

private:
	ServerSocket* _pServerSocket;

	int _nSocketCount;
	int _nAcceptCount;
	int _nCloseCount;
	int _nReciveCount;
	int _nSendCount;
};
