#include <base/MallocManager.h>
#include <protocol/ProtocolBase.h>

#include <socket/AgentServer.h>
#include <socket/ClientIOSocket.h>
#include <socket/ClientSocket.h>
#include <socket/ClientSocketIOThread.h>

AgentServer::AgentServer(ClientIOSocket* socket) : _pSocket(socket)
{
	_nServerType = 0;
	_nLastHeartTime = 0;
}

void AgentServer::tryHeartBeat()
{
	unsigned int now = (unsigned int)time(NULL);
	if (_nLastHeartTime == 0 || now - _nLastHeartTime >= 60)
	{
		Heartbeat xy;
		xy.timestamp = now;

		char *sendbuf = __new(sizeof(Heartbeat));
		bostream bos;
		bos.attach(sendbuf + HEADLEN, sizeof(Heartbeat));
		bos << xy;

		ProtocolHead h;
		h.xyid = xy.XYID;
		h.len = (int)bos.length() + HEADLEN;
		bos.attach(sendbuf, HEADLEN);
		bos << h;
		send(sendbuf, h.len);

		_nLastHeartTime = now;
	}
}

int AgentServer::send(char* buf, unsigned int len)
{
	return _pSocket->getThread()->asynSend(_pSocket, buf, len);
}
