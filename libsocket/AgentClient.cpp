#include <socket/AgentClient.h>
#include <socket/ServerIOSocket.h>
#include <socket/ServerSocket.h>
#include <socket/ServerSocketIOThread.h>

AgentClient::AgentClient(ServerIOSocket* socket) : _pSocket(socket)
{
	_nId = 0;
	_nServerType = 0;
}

int AgentClient::close()
{
	return _pSocket->getThread()->getServerSocket()->closeByLogic(this);
}

int AgentClient::send(char* buf, unsigned int len)
{
	return _pSocket->getThread()->asynSend(_pSocket, buf, len);
}
