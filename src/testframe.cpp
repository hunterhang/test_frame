// testframe.cpp
// jimmymo
// 2012.11.03

#include "testframe.h"
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include "Client.h"

TestCase::TestCase()
{
}

TestCase::~TestCase()
{}

TestFrame::TestFrame()
{
	cpTestCase = NULL;
	cThreadNum = 10;
	cRepeatCount = 0; /* 默认不控制发包次数 */
	cRepeatCountConst = 0;
	cDurationTime = 0;
	cFrequence = 0; /* 默认不控制发包频率 */
	cFrequenceConst = 0;
	cDelayThreshold = 0;

	cTestFrameStatistic = new TestFrameStatistic(this);
}

TestFrame::~TestFrame()
{
}

int TestFrame::Run()
{
	if (cpTestCase == 0) return -1;


	TestFrameThread *threads[cThreadNum];
	int rc;
	int t;

	for(t=0; t < cThreadNum; t++)
	{
		threads[t] = new TestFrameThread(this, t);

		rc = threads[t]->Run();

		if (rc){
			printf("ERROR; return code from TestFrameThread::Run() is %d\n", rc);
			exit(-1);
		}
	}

	// 进入循环前，启动测试，以免第一秒统计结果为零
	// barriar
	sleep(1);
	if (cFrequenceConst > 0)
	{
		cFrequence = cFrequenceConst;
	}
	cCond.Broadcast();//通知其它线程

	// Statistic
	time_t begin;
	time_t lasttime;
	time_t now;

	begin = lasttime = now = time(NULL);

	while (1)
	{
		now = time(NULL);

		if (now - lasttime >= 1)
		{
			{
				// 输出统计结果
				if (now > begin) cTestFrameStatistic->Output();

				// 总量控制
				if (cRepeatCountConst > 0 && cRepeatCount <= 0)
				{
					break;
				}

				// 时长控制
				if ( (cDurationTime > 0) && (now - begin >= cDurationTime) )
				{
					break;
				}

				// 流量控制
				if (cFrequenceConst > 0)
				{
					cLock.Lock();

					cFrequence = cFrequenceConst;
					cCond.Broadcast();
					cLock.Unlock();
				}
			}

			lasttime = now;
		}

		// 稍息，以免CPU过高
		usleep(10);
	}

	//TODO:free threads object

	/* Last thing that main() should do */
	exit(0);

	return 0;
}

int TestFrame::SetTestCase(TestCase *tc)
{
	cpTestCase = tc;
	return 0;
}

int TestFrame::SetThreadNum(int num)
{
	cThreadNum = num;
	return 0;
};

int TestFrame::SetRepeatCount(int count)
{
	cRepeatCountConst = cRepeatCount = count;
	return 0;
};

int TestFrame::SetDurationTime(int seconds)
{
	cDurationTime = seconds;
	return 0;
};

int TestFrame::SetFrequence(int frequence)
{
	cFrequenceConst = frequence;
	return 0;
};

int TestFrame::SetDelayThreshold(int threshold)
{
	cDelayThreshold = threshold;
	return 0;
};



//////////////////////////////////////////////////
//

TestFrameThread::TestFrameThread(TestFrame *tl, int id)
{
	cpTestFrame = tl;
	cID = id;
}

TestFrameThread::~TestFrameThread()
{
}

int TestFrameThread::Run()
{
	//printf("In main: creating thread %ld\n", cID);

	int rc;

	rc = pthread_create(&cThreadID, NULL, TestFrameThread::ThreadFunc, (void *)this);

	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	return 0;
}

int TestFrameThread::RunX()
{
	int rc=0;
	Client client;
	cpTestFrame->cpTestCase->Init(&client);
	// barriar
	cpTestFrame->cLock.Lock();
	rc = cpTestFrame->cpTestCase->PreExecute(&client);
	cpTestFrame->cCond.Wait(cpTestFrame->cLock);
	cpTestFrame->cLock.Unlock();
	if (rc != 0)
	{
		printf("PreExecute fail!%d\n", rc);
		return -1;
	}
	while (1)
	{
		// 流量控制
		if (cpTestFrame->cFrequenceConst > 0)
		{
			cpTestFrame->cLock.Lock();
			if ( cpTestFrame->cFrequence < 0)
			{
				cpTestFrame->cCond.Wait(cpTestFrame->cLock);
			}

			cpTestFrame->cFrequence--;

			// double check
			if ( cpTestFrame->cFrequence < 0)
			{
				cpTestFrame->cLock.Unlock();
				continue;
			}
			cpTestFrame->cLock.Unlock();
		}

		// 控制发包数量
		if (cpTestFrame->cRepeatCountConst > 0 && (cpTestFrame->cRepeatCount--) < 0)
		{
			break;
		}

		rc = Execute(&client);
	}

	return rc;
}

int TestFrameThread::Execute(void *arg)
{
	int rc;

	//clock_t begin, end;

	//begin = clock();
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	// 执行测试案例
	rc = cpTestFrame->cpTestCase->Execute(arg);
	gettimeofday(&end, NULL);
	//end = clock();
	int delay = floor((((end.tv_usec + 1000000 * end.tv_sec) - (begin.tv_usec + 1000000 * begin.tv_sec)))/1000);
	//int delay = (end - begin)*1000/CLOCKS_PER_SEC;
	//int delay = (end - begin);
	cpTestFrame->cTestFrameStatistic->UpdateDelay(delay);

	if (rc == 0)
	{
		cpTestFrame->cTestFrameStatistic->IncreaseSuccessCount();
	}
	else
	{
		cpTestFrame->cTestFrameStatistic->IncreaseFailureCount();
	}

	return rc;
}

void *TestFrameThread::ThreadFunc(void *arg)
{
	TestFrameThread *ptlt = (TestFrameThread *)arg;

	assert(0 != arg);

	ptlt->RunX();

	return NULL;
}

TestFrameStatistic::TestFrameStatistic(TestFrame *tl):cpTestFrame(tl)
{
	pthread_mutex_init(&cMutex, NULL);

	cSuccessCount = 0;
	cFailureCount = 0;
	cTotalCount = 0;
	cTimeoutCount = 0;
	cDelaySum = 0;
	cMaxDelayTime = 0;
	cMinDelayTime = 999999999;

	cSuccessCount_ps = 0;
	cFailureCount_ps = 0;
	cTotalCount_ps = 0;
	cTimeoutCount_ps = 0;
	cDelaySum_ps = 0;
	cMaxDelayTime_ps = 0;
	cMinDelayTime_ps = 99999999;
}

TestFrameStatistic::~TestFrameStatistic()
{
	pthread_mutex_destroy(&cMutex);
}

void TestFrameStatistic::IncreaseSuccessCount()
{
	pthread_mutex_lock(&cMutex);
	cSuccessCount ++;
	cSuccessCount_ps ++;
	cTotalCount ++;
	cTotalCount_ps ++;
	pthread_mutex_unlock(&cMutex);
}

void TestFrameStatistic::IncreaseFailureCount()
{
	pthread_mutex_lock(&cMutex);
	cFailureCount ++;
	cFailureCount_ps ++;
	cTotalCount ++;
	cTotalCount_ps ++;
	pthread_mutex_unlock(&cMutex);
}

void TestFrameStatistic::UpdateDelay(int delay)
{
	pthread_mutex_lock(&cMutex);
	cDelaySum += delay;
	cDelaySum_ps += delay;

	if (delay > cMaxDelayTime)
	{
		cMaxDelayTime = delay;
	}
	if (delay < cMinDelayTime)
	{
		cMinDelayTime = delay;
	}
	if (delay > cMaxDelayTime_ps)
	{
		cMaxDelayTime_ps = delay;
	}
	if (delay < cMinDelayTime_ps)
	{
		cMinDelayTime_ps = delay;
	}

	if(cpTestFrame->cDelayThreshold && delay > cpTestFrame->cDelayThreshold)
	{
		cTimeoutCount++;
		cTimeoutCount_ps++;
	}

	pthread_mutex_unlock(&cMutex);
}

void TestFrameStatistic::Output()
{
	time_t now = time(NULL);
	char tmpbuf[128];
	strftime(tmpbuf, sizeof(tmpbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

	printf("##################################################################################################\n");
	printf("[%s]\n", tmpbuf);
	printf("success/s:%d failure/s:%d total/s:%d delayHealth/s:%2.2f%% delayAvg/s:%d ms maxDelay/s:%d ms minDelay/s:%d ms\n",
			(int)cSuccessCount_ps, (int)cFailureCount_ps, (int)cTotalCount_ps,
			cTotalCount_ps ? (cTotalCount_ps - cTimeoutCount_ps)*100.00/cTotalCount_ps : 100.00,
			(int)cTotalCount_ps ? (int)cDelaySum_ps/(int)cTotalCount_ps:0, cMaxDelayTime_ps, cMinDelayTime_ps
			);
	printf("success:%d failure:%d total:%d delayHealth:%2.2f%% delayAvg:%llu ms maxDelay:%d ms minDelay:%d ms\n",
			(int)cSuccessCount, (int)cFailureCount, (int)cTotalCount,
			cTotalCount ? (cTotalCount - cTimeoutCount)*100.00/cTotalCount : 100.00,
			cTotalCount ? cDelaySum/cTotalCount:0, cMaxDelayTime, cMinDelayTime
			);

	pthread_mutex_lock(&cMutex);
	cSuccessCount_ps = 0;
	cFailureCount_ps = 0;
	cTotalCount_ps = 0;
	cTimeoutCount_ps = 0;
	cDelaySum_ps = 0;
	cMaxDelayTime_ps = 0;
	cMinDelayTime_ps = 99999999;
	pthread_mutex_unlock(&cMutex);
}
