#pragma once

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <socket/SocketBase.h>
#include <socket/ClientIOSocket.h>

class AgentServer : public unCopyable
{
public:
	AgentServer(class ClientIOSocket* socket);
	
public:
	/**
	 * 发送心跳保持连接
	 * 内部控制每60秒发一次
	 */
	void tryHeartBeat();

	/**
	 * 发送数据
	 * 逻辑层调用的接口
	 */
	int send(char* buf, unsigned int len);

public:
	ClientIOSocket* getSocket() { return _pSocket; }
	inline int getID(){ return _pSocket->getID(); };
	
	inline void setServerType(int type) { _nServerType = type; };
	inline int getServerType() { return _nServerType; };
	
private:
	ClientIOSocket* _pSocket;
	int _nServerType;
	unsigned int _nLastHeartTime;
};
