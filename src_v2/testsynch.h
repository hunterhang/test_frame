#ifndef TESTSYNCH_H_2012
#define TESTSYNCH_H_2012


#include <pthread.h>

typedef unsigned long long Time_t;

class MutexLock
{
	public:
		MutexLock();
		~MutexLock();

		void Lock(void);
		void Unlock(void);

	private:
		pthread_mutex_t ctMutex;

		friend class Condition;
};

class SmartLock
{
	public:
	SmartLock(MutexLock& lock);
	~SmartLock();

	private:
	MutexLock& coLock;
};

// multi-thread counter
class AtomicCounter
{
public:
    AtomicCounter( int count = 0 ) : ciCount(count) {}
    
    AtomicCounter& operator = ( int n) 
    {
		SmartLock loLock(coLock);
        ciCount = n; 
        return *this;
    }
    
    int operator ++ ( int )
    {
		SmartLock loLock(coLock);
        int liTmp = ++ciCount;
        return liTmp;
    }
    
    int operator -- ( int )
    {
		SmartLock loLock(coLock);
        int liTmp = --ciCount;
        return liTmp;
    }

	operator int ()
	{
		SmartLock loLock(coLock);
		int liTmp = ciCount;
		return liTmp;
	}

protected:
	int ciCount;
	MutexLock coLock;
};
    
class Condition
{
public:
    Condition();       ///<initialize condition variables
    ~Condition();                           ///<destroy condition variables

    /**
     * @brief  Wait on a condition.
     *
     * @param aoMutex        IN - a mutex
     * 
     * @return 0:upon successful completion;-1:error
     */
    int Wait(MutexLock& aoMutex);

    /**
     * @brief Timedwait on a condition.
     *
     * @param aoMutex        IN - a mutex
     * @param aiTimeout      IN - timeout
     *
     * @return 0:upon successful completion;-1:timeout
     */
    int WaitFor(MutexLock& aoMutex, Time_t aiTimeout);

    /**
     * @brief Timedwait on a condition
     *
     * @param aoMutex        IN - a mutex
     * @param aiTimeout      IN - timeout
     *
     * @return 0:upon successful completion;-1:timeout
     */
    int WaitUntil(MutexLock& aoMutex, Time_t aiExpired);

    /**
     * @brief Signal a condition.
     *
     * @return Return 0 if successful;otherwise,an error number shall be returned to indicate the error.
     */
    int Signal(void);

    /**
     * @brief Broadcast a condition.
     *
     * @return Return 0 if successful;otherwise,an error number shall be returned to indicate the error.
     */
    int Broadcast(void);

private:
    pthread_cond_t  ctCond;
};


#endif
