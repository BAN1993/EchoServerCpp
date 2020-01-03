#include <base/Base.h>
#include <base/Lock.h>
#include <base/Condition.h>
#include <base/Utils.h>

#if defined _WINDOWS_ || defined WIN32

Condition::Condition(void)
{
	m_condid = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	assert(m_condid != INVALID_HANDLE_VALUE);
}

Condition::~Condition(void)
{
	if (m_condid != INVALID_HANDLE_VALUE)
		::CloseHandle(m_condid);
}

bool Condition::wait(Lock& locker)
{
	bool ret = false;
	locker.unlock();
	if (::WaitForSingleObject(m_condid, INFINITE) == WAIT_OBJECT_0)
		ret = true;
	locker.lock();

	::ResetEvent(m_condid);

	return ret;
}

bool Condition::wait(Lock& locker, unsigned int ms)
{
	bool ret = false;
	locker.unlock();
	if (::WaitForSingleObject(m_condid, ms) == WAIT_OBJECT_0)
		ret = true;
	locker.lock();

	::ResetEvent(m_condid);

	return ret;
}

void Condition::signal(void)
{
	::SetEvent(m_condid);
}

void Condition::broadcast(void)
{
	::SetEvent(m_condid);
}

#else

Condition::Condition(void)
{
	int  rc = ::pthread_cond_init(&m_condid,0);
	assert( rc == 0 );
}

Condition::~Condition (void)
{
	::pthread_cond_destroy(&m_condid);
}

bool Condition::wait (Lock& locker)
{
	return ::pthread_cond_wait(&m_condid, &locker.m_crit)==0;
}

bool Condition::wait(Lock& locker, unsigned int ms)
{
	unsigned long long expires64 = getTimeStampMsec() + ms;
	timespec expiresTS;
	expiresTS.tv_sec = expires64 / 1000;
	expiresTS.tv_nsec = (expires64 % 1000) * 1000000L;

	return ::pthread_cond_timedwait(&m_condid, &locker.m_crit, &expiresTS)==0;
}

void Condition::signal (void)
{
	::pthread_cond_signal(&m_condid);
}

void Condition::broadcast(void)
{
	::pthread_cond_broadcast(&m_condid);
}

#endif
