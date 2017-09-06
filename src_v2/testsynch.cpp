#include "testsynch.h"

#include <assert.h>
#include <sys/time.h>
#include <errno.h>

MutexLock::MutexLock()
{
	int rc = pthread_mutex_init(&ctMutex, NULL);
	assert(0 == rc);
}

MutexLock::~MutexLock()
{
	int rc = pthread_mutex_destroy(&ctMutex);
	assert(0 == rc);
}

void MutexLock::Lock()
{
	int rc = pthread_mutex_lock(&ctMutex);
	assert(0 == rc);
}

void MutexLock::Unlock()
{
	int rc = pthread_mutex_unlock(&ctMutex);
	assert(0 == rc);
}

SmartLock::SmartLock(MutexLock& lock):coLock(lock)
{
	coLock.Lock();
}

SmartLock::~SmartLock()
{
	coLock.Unlock();
}

Condition::Condition()
{
    int liRetCode;

	liRetCode = pthread_cond_init(&ctCond, NULL);
	assert(0 == liRetCode);
}
 
Condition::~Condition()
{
    int liRetCode = pthread_cond_destroy(&ctCond); 
    assert(0 == liRetCode);
}
 
int Condition::Wait(MutexLock& aoMutex)
{
    int liRetCode;
 
    liRetCode = pthread_cond_wait(&ctCond, &(aoMutex.ctMutex));
 
    return (0 == liRetCode ? 0 : -1);
}
 
int Condition::WaitFor(MutexLock& aoMutex, Time_t aiTimeout)
{
	struct timeval ltTV;
	Time_t ltExpired;

	gettimeofday(&ltTV, NULL);

	ltExpired = ltTV.tv_sec*1000*1000+ltTV.tv_usec + aiTimeout;

    return WaitUntil(aoMutex, ltExpired);
}
 
int Condition::WaitUntil(MutexLock& aoMutex, Time_t aiExpired)
{
    int         liRetCode;
 
    timespec loTS;
	loTS.tv_sec = aiExpired/(1000*1000);
	loTS.tv_nsec = ( aiExpired%(1000*1000) ) * 1000;
    
    liRetCode = pthread_cond_timedwait(&ctCond, &(aoMutex.ctMutex), &loTS);
 
    assert(EINVAL != liRetCode);
 
    return (ETIMEDOUT == liRetCode ? -1 : 0);
}
 
int Condition::Signal()
{
    int liRetCode = pthread_cond_signal(&ctCond);
    assert(0 == liRetCode);
 
    return liRetCode;
}
 
int Condition::Broadcast()
{
    int liRetCode = pthread_cond_broadcast(&ctCond);
    assert(0 == liRetCode);
 
    return liRetCode;
}
