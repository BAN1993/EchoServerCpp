#pragma once

#include <tool/Uncopyable.h>

class Lock;

class Condition : public unCopyable
{
#if defined _WINDOWS_ || defined WIN32
	typedef HANDLE			condid;
#else
	typedef pthread_cond_t	condid;
#endif

public:
	explicit  Condition(void);
	~Condition(void);

	/**
	 * 等待信号到达
	 * ms为超时时间，单位为毫秒
	 */
	bool wait(Lock& locker);
	bool wait(Lock& locker, unsigned int ms);

	/**
	 * 发送信号
	 * 通知线程关闭
	 * 在windows上无区别
	 */
	void signal(void);
	void broadcast(void);

private:
	condid	m_condid;
};
