#include <base/Thread.h>
#include <base/Log.h>

#if defined _WINDOWS_ || defined WIN32
static unsigned int __stdcall threadFunc(void *pval)
{
	Thread* pThis = static_cast<Thread*>(pval);
	assert(pThis);

	std::string threadname = pThis->getName();
	LOGSYS("running:threadname=" << threadname);

	pThis->run();
	pThis->broadcast();

	::_endthreadex(0);

	return 0;
}
#else
extern "C"
{
	static void* threadFunc(void *pval)
	{
		Thread* pThis = static_cast<Thread*>(pval);
		assert(pThis);

		std::string threadname = pThis->getName();
		LOGSYS("running:threadname="<<threadname);

		pThis->run( );
		pThis->broadcast( );

		::pthread_exit(nullptr);

		return 0;
	}
}
#endif

Thread::Thread(const std::string &name/*=""*/)
	: m_hThread(INVALID_THREAD_VALUE)
	, m_name(name)
	, m_brunning(false)
	, m_lock()
	, m_condition()
{

}

Thread::~Thread(void)
{
	terminate(1000);
}

bool Thread::start(void)
{
	SelfLock l(m_lock);
	if (m_hThread != INVALID_THREAD_VALUE || m_brunning)
	{
		LOGSYS("failed to start:threadname=" << m_name << ",m_brunning=" << m_brunning);
		return false;
	}

	m_brunning = true;
	int err = 0;

#if defined _WINDOWS_ || defined WIN32
	unsigned int threadID = 0;
	m_hThread = (HANDLE)_beginthreadex(0, 0, threadFunc, (void*)this, 0, &threadID);
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		err = GetLastError();

		LOGSYS("create failed:threadname=" << m_name << ",err=" << err);

		m_brunning = false;

		return false;
	}

	return true;
#else
	int retval = pthread_create(&m_hThread, 0, threadFunc, (void*)this);
	if (retval != 0)
	{
		err = retval;

		LOGSYS("create failed:threadname=" << m_name << ",err=" << err);

		m_brunning = false;

		return false;
	}

	pthread_detach(m_hThread);

	return true;
#endif
}

void Thread::broadcast(void)
{
	SelfLock l(m_lock);
	m_condition.broadcast();
}

void Thread::terminate(unsigned int ms)
{
	SelfLock l(m_lock);

	if (m_brunning)
	{
		m_brunning = false;
		if (!m_condition.wait(m_lock, ms))
		{
			if (m_hThread != INVALID_THREAD_VALUE)
			{
				//#if defined _WINDOWS_ || defined WIN32
				//				if ( GetCurrentThread() == m_hThread )	::_endthreadex( 0 );
				//				else									::TerminateThread(m_hThread, 0);
				//#else
				//				if ( pthread_self() == m_hThread )		::pthread_exit(nullptr);
				//				else									::pthread_cancel(m_hThread);
				//#endif
			}
		}

#if defined _WINDOWS_ || defined WIN32
		if (m_hThread != INVALID_THREAD_VALUE)			::CloseHandle(m_hThread);
#endif
		m_hThread = INVALID_THREAD_VALUE;
	}
}

std::string	Thread::getName() const
{
	return m_name;
}

bool Thread::isRunning(void)
{
	return m_brunning;
}

bool Thread::isStop(void)
{
	return !m_brunning;
}
