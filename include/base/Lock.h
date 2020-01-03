#pragma once

#include <base/Base.h>
#include <tool/Uncopyable.h>

class Lock : public unCopyable
{
	friend class Condition;
public:
	explicit Lock(void);
	~Lock(void);

	void lock(void);

	int trylock(void);

	void unlock(void);

private:
#if defined _WINDOWS_ || defined WIN32
	CRITICAL_SECTION	m_crit;
#else
	pthread_mutex_t		m_crit;
#endif
};

class SelfLock : public unCopyable
{
public:
	explicit SelfLock(Lock &lock);
	~SelfLock(void);

private:
	Lock &m_lock;
};
