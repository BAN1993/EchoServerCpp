#pragma once

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <socket/SocketBase.h>
#include <socket/ServerIOSocket.h>

class AgentClient : public unCopyable
{
public:
	AgentClient(class ServerIOSocket* socket);
	
public:
	/**
	* 主动关闭
	* 逻辑层调用的接口
	*/
	int close();

	/**
	* 发送数据
	* 逻辑层调用的接口
	*/
	int send(char* buf, unsigned int len);

public:
	ServerIOSocket* getSocket() { return _pSocket; }
	inline unsigned long long getFd(){ return _pSocket->getFd(); }; // socketfd
	inline sockaddr_in getAddr() { return _pSocket->getAddr(); };

	/**
	 * id标识
	 * 若是服务则为服务id,若是客户端则为numid
	 */
	inline void setID(int id){ _nId = id; };
	inline int getID(){ return _nId; };

	inline void setServerType(int type) { _nServerType = type; };
	inline int getServerType() { return _nServerType; };

private:

private:
	ServerIOSocket* _pSocket;
	int _nId;
	int _nServerType;
};
