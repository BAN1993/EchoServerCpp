#include <base/Base.h>
#include <base/Lock.h>

Lock::Lock(void)
{
#if defined _WINDOWS_ || defined WIN32
	::InitializeCriticalSection(&m_crit);
#else
	int ret = ::pthread_mutex_init(&m_crit, 0);
	assert(ret==0);
#endif	
};

Lock::~Lock(void)
{
#if defined _WINDOWS_ || defined WIN32
	::DeleteCriticalSection(&m_crit);
#else
	int ret = ::pthread_mutex_destroy(&m_crit);

	assert( ret != EBUSY );
	assert( ret == 0 );
#endif
}

void Lock::lock(void)
{
#if defined _WINDOWS_ || defined WIN32
	::EnterCriticalSection(&m_crit);
#else
	int rc = ::pthread_mutex_lock(&m_crit);

	assert( rc != EINVAL );
	assert( rc != EDEADLK );
	assert( rc == 0 );
#endif
}

int Lock::trylock(void)
{
#if defined _WINDOWS_ || defined WIN32
	return ::TryEnterCriticalSection(&m_crit);
#else
	return ::pthread_mutex_trylock(&m_crit);
#endif
	return -1;
}

void Lock::unlock(void)
{
#if defined _WINDOWS_ || defined WIN32
	::LeaveCriticalSection(&m_crit);
#else
	int rc = ::pthread_mutex_unlock(&m_crit);

	assert( rc != EINVAL );
	assert( rc != EPERM );
	assert( rc == 0 );
#endif
};

SelfLock::SelfLock(Lock &lock) : m_lock(lock)
{
	m_lock.lock();
}

SelfLock::~SelfLock(void)
{
	m_lock.unlock();
}
