#pragma once

/*=============================================
=	线程模块
	支持linux和windows

==============================================*/

#include <tool/Uncopyable.h>
#include <string>
#include <base/Lock.h>
#include <base/Condition.h>

class Thread
{
#if defined _WINDOWS_ || defined WIN32
#define INVALID_THREAD_VALUE INVALID_HANDLE_VALUE
	typedef HANDLE				THREADID;
#else
#define INVALID_THREAD_VALUE 0
	typedef pthread_t			THREADID;
#endif

	DEFINE_UNCOPYABLE(Thread);

public:
	Thread(const std::string &name = "");
	virtual ~Thread(void);

public:
	/**
	 * 循环函数,必须继承
	 */
	virtual int run() = 0;

	/**
	 * 线程开始
	 */
	bool start(void);

	/**
	 * 通知本线程关闭
	 */
	void broadcast(void);
	
	/**
	 * 线程结束
	 * ms:超时时间(毫秒),如果超时则强行关闭
	 */
	void terminate(unsigned int ms = 500);

public:
	std::string	getName() const;
	bool isRunning(void);
	bool isStop(void);
	
protected:
	THREADID			m_hThread;
	const std::string	m_name;
	volatile bool		m_brunning;
	Lock				m_lock;
	Condition			m_condition;
};
