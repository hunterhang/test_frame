// testframe.h
// jimmymo
// 2012.11.03

#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <math.h>
#include "../tcinclude/tc_singleton.h"
#include "../tcinclude/tc_http.h"
#include "../tcinclude/tc_common.h"
#include "../tcinclude/tc_epoller.h"
#include "testsynch.h"
#include "Client.h"

const unsigned int LEN_MAXRECV = 4096;
class TestFrameStatistic;


// 测试案例基类
class TestCase
{
	public:
		TestCase();
		virtual ~TestCase();

		virtual int Init(const std::string &ip, const unsigned short &port) = 0;

		/* 测试框架回调的执行函数       *
		 * @brief 请在继承类编写实际的测试案例 */
		virtual int Execute() = 0;
		virtual int PreExecute() = 0;
};

struct EndPoint {
	std::string _ip;
	unsigned short _port;
};

// 测试库
class TestFrame
{
	public:
		TestFrame();
		~TestFrame();
		// 设置测试案例对象
		int addObject(TestCase *tc);
		// 运行测试
		int Run();
		static void *ThreadFunc(void *argv);
		void SetThreadNum(unsigned int conn_num);
		void SetDurationTime(unsigned int durationTime);
		void SetFrequence(unsigned int frequence);
		
	public:
		TestCase *cpTestCase;
		unsigned int _cHandleNum;
		unsigned int _cDurationTime;
		unsigned int _cFrequenceConst;
		AtomicCounter cFrequence;
		AtomicCounter cRepeatCount;
		std::vector<TestCase*> vTestCase;
		std::vector<EndPoint> _vEndPoint;
		MutexLock cLock;
		Condition cCond;

};


class TestFrameStatistic
{
	public:
		TestFrameStatistic();
		~TestFrameStatistic();

		void IncreaseSuccessCount();
		void IncreaseFailureCount();
		void UpdateDelay(int delay);
		void Output();

		//TestFrame *cpTestFrame;

		pthread_mutex_t cMutex;

		int cSuccessCount;
		int cFailureCount;
		unsigned long long cTotalCount;
		int cTimeoutCount;
		unsigned long long cDelaySum;
		int cMaxDelayTime;
		int cMinDelayTime;
		int cTotalSecond;

		int cSuccessCount_ps;
		int cFailureCount_ps;
		int cTotalCount_ps;
		int cTimeoutCount_ps;
		int cDelaySum_ps;
		int cMaxDelayTime_ps;
		int cMinDelayTime_ps;
};





struct ReqQueueItem
{
	std::string _req;
	std::string _assert;
	timeval _tv;
};

class ReqQueue {
public:
	ReqQueue() {};
	void insert(const std::string &req_id, const std::string &req, const std::string &assert)
	{
		SmartLock lock(_lock);
		timeval tv;
		gettimeofday(&tv, NULL);
		ReqQueueItem req_info;
		req_info._tv = tv;
		req_info._req = req;
		req_info._assert = assert;
		_pool.insert(make_pair(req_id, req_info));
		//printf("insert:%s\n",req_id.c_str());
	}

	int find(const std::string &req_id, ReqQueueItem &req_item, unsigned int &timecost)
	{
		SmartLock lock(_lock);
		timeval tv;
		gettimeofday(&tv, NULL);
		std::map<std::string, ReqQueueItem>::iterator it = _pool.find(req_id);
		if (it == _pool.end())
		{
			return -1;
		}
		timecost = floor((((tv.tv_usec + 1000000 * tv.tv_sec) - (it->second._tv.tv_usec + 1000000 * it->second._tv.tv_sec))) / 1000);
		req_item = it->second;

		//删除当前对象
		_pool.erase(it);
		//printf("find:%s\n", req_id.c_str());
		return 0;
	}
	void print(const unsigned int &timeout)
	{
		SmartLock lock(_lock);
		printf("[%s] the size of none rsp is %zu\n", taf::TC_Common::tm2str(time(NULL), "%Y-%m-%d %H:%M:%S").c_str(),_pool.size());
		return;
		timeval tv;
		gettimeofday(&tv, NULL);
		unsigned int timeout_num = 0;
		std::map<std::string, ReqQueueItem>::iterator it = _pool.begin();
		for (; it != _pool.end(); it++)
		{
			if ((((tv.tv_usec + 1000000 * tv.tv_sec) - (it->second._tv.tv_usec + 1000000 * it->second._tv.tv_sec))) >= timeout)
			{
				++timeout_num;
				//printf("timeout:%s,count:%zu,sec:%ld,uec:%ld\n", it->second._req.c_str(),_pool.size(),it->second._tv.tv_sec,it->second._tv.tv_usec);
			}
		}
		printf("timeout_num:%u,size:%zu\n", timeout_num, _pool.size());
	}

	std::map<std::string, ReqQueueItem> _pool;
private:
	MutexLock _lock;
};


