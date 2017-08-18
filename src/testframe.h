// testframe.h
// jimmymo
// 2012.11.03

#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include "testsynch.h"

class TestFrameStatistic;

// 测试案例基类
class TestCase
{
	public:
		TestCase();
		virtual ~TestCase();

		virtual int Init(void *arg = 0) = 0;

		/* 测试框架回调的执行函数       *
		 * @brief 请在继承类编写实际的测试案例 */
		virtual int Execute(void *arg = 0) = 0;
		virtual int PreExecute(void *arg = 0) = 0;
};

// 测试库
class TestFrame
{
	public:
		TestFrame();
		~TestFrame();

		// 运行测试
		int Run();

		// 设置测试案例对象
		int SetTestCase(TestCase *tc);

		// 设置线程数
		int SetThreadNum(int num);

		// 设置重复次数
		int SetRepeatCount(int count);

		// 设置持续时间，默认无限时长
		int SetDurationTime(int seconds);

		// 设置发包频率，默认不控制发包频率
		int SetFrequence(int frequence);

		// 设置阀值，单位：毫秒
		int SetDelayThreshold(int threshold/*ms*/);

		void *ThreadFunc(void *arg);

		TestCase *cpTestCase;

		TestFrameStatistic *cTestFrameStatistic;

		int cDelayThreshold;

	private:
		int cThreadNum;
		int cRepeatCountConst;
		int cDurationTime;
		int cFrequenceConst;
		AtomicCounter cFrequence;
		AtomicCounter cRepeatCount;

		MutexLock cLock;
		Condition cCond;
	
	friend class TestFrameThread;
};

class TestFrameThread
{
	public:
		TestFrameThread(TestFrame *tl, int id);
		~TestFrameThread();

		int Run();
		int RunX();

		int Execute(void *arg = 0);
		
		static void *ThreadFunc(void *arg);

		TestFrame *cpTestFrame;

	private:
		int cID;
		pthread_t cThreadID;
};

class TestFrameStatistic
{
	public:
		TestFrameStatistic(TestFrame *tl);
		~TestFrameStatistic();

		void IncreaseSuccessCount();
		void IncreaseFailureCount();
		void UpdateDelay(int delay);

		void Output();

		TestFrame *cpTestFrame;

		pthread_mutex_t cMutex;

		int cSuccessCount;
		int cFailureCount;
		unsigned long long cTotalCount;
		int cTimeoutCount;
		unsigned long long cDelaySum;
		int cMaxDelayTime;
		int cMinDelayTime;

		int cSuccessCount_ps;
		int cFailureCount_ps;
		int cTotalCount_ps;
		int cTimeoutCount_ps;
		int cDelaySum_ps;
		int cMaxDelayTime_ps;
		int cMinDelayTime_ps;
};

