// testframe.cpp
// jimmymo
// 2012.11.03

#include "testframe.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include "Client.h"
#include "log.h"
#include "../tcinclude/tc_epoller.h"

extern taf::TC_Epoller g_epoll;
extern Condition g_thread_cond;
extern MutexLock g_lock;
TestCase::TestCase()
{
}

TestCase::~TestCase()
{}

TestFrame::TestFrame()
{
	_cHandleNum = 1;
	_cDurationTime = 0;
	_cFrequenceConst = 0;
}

TestFrame::~TestFrame()
{

}

void *TestFrame::ThreadFunc(void *argv)
{
	
	int nRet = -1;
	TestFrame *cpTestFrame = (TestFrame *)argv;
	//battery
	cpTestFrame->cLock.Lock();
	cpTestFrame->cCond.Wait(cpTestFrame->cLock);
	cpTestFrame->cLock.Unlock();
	while (1)
	{
		//如果总数大于线程数，则每个都平均发
		if (cpTestFrame->_cFrequenceConst >= cpTestFrame->vTestCase.size())
		{
			unsigned int add = 0;
			for (size_t i = 0; i < cpTestFrame->vTestCase.size(); i++)
			{
				for (unsigned int j = 0; j < floor(cpTestFrame->_cFrequenceConst/cpTestFrame->vTestCase.size()); j++)
				{
					nRet = cpTestFrame->vTestCase[i]->Execute();
					if (nRet != 0)
					{
						printf("TestCase Execute fail!nRet=%d\n", nRet);
						continue;
					}
					add++;
				}
			}
			printf("[%s] finished send,total:%u\n", taf::TC_Common::tm2str(time(NULL),"%Y-%m-%d %H:%M:%S").c_str(),add);
		}
		else {
			unsigned int add = 0;
			srand(time(NULL));
			int rand_num = rand();
			for (size_t i = 0; i < cpTestFrame->_cFrequenceConst; i++)
			{
				rand_num++;
				unsigned int mode = rand_num % cpTestFrame->vTestCase.size();
				nRet = cpTestFrame->vTestCase[mode]->Execute();
				if (nRet != 0)
				{
					printf("TestCase1 Execute fail!nRet=%d\n", nRet);
					continue;
				}
				add++;
			}
			printf("[%s] finished send,total:%u\n", taf::TC_Common::tm2str(time(NULL), "%Y-%m-%d %H:%M:%S").c_str(),add);
		}
		//通知收请求了
		//第1次
		g_thread_cond.Broadcast();

		cpTestFrame->cLock.Lock();
		cpTestFrame->cCond.Wait(cpTestFrame->cLock);
		cpTestFrame->cLock.Unlock();
	}
	
	return NULL;
}

int TestFrame::Run()
{
	//初始化
	for (size_t i = 0; i < vTestCase.size(); i++)
	{
		int nRet = vTestCase[i]->PreExecute();
		if (nRet != 0)
		{
			printf("TestCase PreExecute fail!index:%zu,nRet=%d\n", i, nRet);
			return -2;
		}
		else
		{
			//printf("TestCase PreExecute success\n");
		}
	}

	int rc;
	pthread_t cThreadID;
	rc = pthread_create(&cThreadID, NULL, TestFrame::ThreadFunc, (void *)this);

	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	if (_cFrequenceConst > 0)
	{
		cFrequence = _cFrequenceConst;
	}
	// 进入循环前，启动测试，以免第一秒统计结果为零
	// barriar
	//刚进入第1秒时，再启动测试
	time_t begin;
	time_t lasttime;
	time_t now;
	begin = lasttime = now = time(NULL);
	while (time(NULL) != (now + 1))
	{
		usleep(2);
	}
	cCond.Broadcast();//通知其它线程
	while (1)
	{
		now = time(NULL);

		if (now - lasttime >= 1)
		{
			{
				// 输出统计结果
				//if (now > begin) 	taf::TC_Singleton<TestFrameStatistic>::getInstance()->Output(); 

				// 时长控制
				if ( (_cDurationTime > 0) && (now - begin >= _cDurationTime) )
				{
					break;
				}
				// 流量控制
				//if (cFrequenceConst > 0)
				//{
					//printf("Broadcast,%lu,lasttime:%lu\n", now,lasttime);
					
					cLock.Lock();
					cFrequence = _cFrequenceConst;
					cCond.Broadcast();
					cLock.Unlock();
				//}
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

int TestFrame::addObject(TestCase *tc)
{
	vTestCase.push_back(tc);
	return 0;
}
void TestFrame::SetThreadNum(unsigned int conn_num)
{
	_cHandleNum = conn_num;
}
void TestFrame::SetDurationTime(unsigned int durationTime)
{
	_cDurationTime = durationTime;
}
void TestFrame::SetFrequence(unsigned int frequence)
{
	_cFrequenceConst = frequence;
}


TestFrameStatistic::TestFrameStatistic()
{
	pthread_mutex_init(&cMutex, NULL);

	cSuccessCount = 0;
	cFailureCount = 0;
	cTotalCount = 0;
	cTimeoutCount = 0;
	cDelaySum = 0;
	cMaxDelayTime = 0;
	cMinDelayTime = 999999999;
	cTotalSecond = 0;

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
	/**
	if(cpTestFrame->cDelayThreshold && delay > cpTestFrame->cDelayThreshold)
	{
		cTimeoutCount++;
		cTimeoutCount_ps++;
	}
	**/
	pthread_mutex_unlock(&cMutex);
}

void TestFrameStatistic::Output()
{
	cTotalSecond++;
	time_t now = time(NULL);
	char report_buf[2048] = { 0 };
	char tmpbuf[128];
	strftime(tmpbuf, sizeof(tmpbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

	int pos = snprintf(report_buf,sizeof(report_buf),"##################################################################################################\n");
	pos += snprintf(report_buf+pos,sizeof(report_buf) - pos,"[%s]\n", tmpbuf);
	pos += snprintf(report_buf+pos,sizeof(report_buf) - pos,"success/s:%d failure/s:%d total/s:%d delayHealth/s:%2.2f%% delayAvg/s:%d ms maxDelay/s:%d ms minDelay/s:%d ms\n",
			(int)cSuccessCount_ps, (int)cFailureCount_ps, (int)cTotalCount_ps,
			cTotalCount_ps ? (cTotalCount_ps - cTimeoutCount_ps)*100.00/cTotalCount_ps : 100.00,
			(int)cTotalCount_ps ? (int)cDelaySum_ps/(int)cTotalCount_ps:0, cMaxDelayTime_ps, cMinDelayTime_ps
			);
	pos += snprintf(report_buf + pos,sizeof(report_buf) - pos,"success:%d failure:%d total:%d qps:%d delayHealth:%2.2f%% delayAvg:%llu ms maxDelay:%d ms minDelay:%d ms\n",
			(int)cSuccessCount, (int)cFailureCount, (int)cTotalCount,(int)floor(cTotalCount/cTotalSecond),
			cTotalCount ? (cTotalCount - cTimeoutCount)*100.00/cTotalCount : 100.00,
			cTotalCount ? cDelaySum/cTotalCount:0, cMaxDelayTime, cMinDelayTime
			);
	printf("%s\n", report_buf);
	taf::TC_Singleton<log_file>::getInstance()->log_error(report_buf);
	
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
