#include <stdio.h>
#include <string>
#include <math.h>

// 包含测试框架头文件
#include "testframe.h"

#include "Client.h"
#include "../tcinclude/tc_http.h"
#include "../tcinclude/tc_singleton.h"
#include "../tcinclude/tc_common.h"
#include "../tcinclude/tc_epoller.h"
#include "loadConfig.h"
#include "testsynch.h"
#include "log.h"
#include <sys/time.h>
#include <sys/epoll.h>
//static pthread_mutex_t *lockarray;

#define MAX_SDK_BUFFER_SIZE 1024
#define QUERY_SERVICE(cmd) client->RPC<cmd>(req, rsp)
#define QUERY_HTTP() client->http(httpBuffer,rspBuffer)
const unsigned int LEN_MAXRECV = 4096;

taf::TC_Epoller g_epoll(true);


using namespace std;

class UserAccountPool {
public:
	UserAccountPool() {};
	void Init(unsigned long long num)
	{
		_num = num;
	}
	std::string getIncre() {
		SmartLock lock(_lock);
		_num++;
		std::stringstream ss;
		ss
			<< _num;
		return ss.str();
	}
private:
	MutexLock _lock;
	unsigned long long _num;
};

class ReqIdPool {
public:
	ReqIdPool() :_num(1) {};
	std::string getIncre() {
		SmartLock lock(_lock);
		_num++;
		std::stringstream ss;
		ss
			<< _num;
		return ss.str();
	}
private:
	MutexLock _lock;
	unsigned long long _num;
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
	void insert(const std::string &req_id, const std::string &req,const std::string &assert)
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
		printf("timeout_num:%u,size:%zu\n", timeout_num,_pool.size());
	}

	std::map<std::string, ReqQueueItem> _pool;
private:
	MutexLock _lock;
};

class HandleInput {
public:
	void insert(unsigned short sock, const std::string &str)
	{
		SmartLock lock(_lock);
		_pool.insert(make_pair(sock, str));
	};
	int find(unsigned short sock, std::map<unsigned short, std::string>::iterator &it)
	{
		it = _pool.find(sock);
		if (it == _pool.end())
		{
			return -1;
		}
		//it= it->second;
		return 0;
	}
	int update(std::map<unsigned short, std::string>::iterator &it, const std::string &str)
	{
		//_pool[sock] = str;
		it->second = str;
		return 0;
	}
private:
	std::map<unsigned short, std::string> _pool;
	MutexLock _lock;
};

class ReqPool {
public:
	void Init(const std::vector<ReqInfo> &req_list)
	{
		_req_list = req_list;
	}
	ReqInfo getReq(std::string &user_id, std::string &family_id, std::string &token, std::string &phone)
	{
		struct timeval tv;
		gettimeofday(&tv, 0);
		srand(tv.tv_usec + atoi(user_id.c_str()));
		unsigned int index = rand() % _req_list.size();
		if (index == 0)
		{
			index = 1;
		}
		std::string req_id = taf::TC_Singleton<ReqIdPool>::getInstance()->getIncre();
		ReqInfo req_info;
		req_info = _req_list[index - 1];
		req_info._req = taf::TC_Common::replace(req_info._req, "$user_id", user_id);
		req_info._req = taf::TC_Common::replace(req_info._req, "$family_id", family_id);
		req_info._req = taf::TC_Common::replace(req_info._req, "$token", token);
		req_info._req = taf::TC_Common::replace(req_info._req, "$req_id", req_id);
		req_info._req = taf::TC_Common::replace(req_info._req, "$phone", phone);
		taf::TC_Singleton<ReqQueue>::getInstance()->insert(req_id, req_info._req,req_info._assert);
		return req_info;
	}
	static int getVal(const std::string &rsp, const std::string &name, std::string &val, bool is_number = true)
	{
		size_t pos = 2;
		std::string ends = ",";
		size_t name_len = name.size();
		if (!is_number)
		{
			pos = 3;
			ends = "\"";
		}
		size_t p1 = rsp.find(name);
		if (p1 == std::string::npos)
		{
			return -1;
		}
		size_t p2 = rsp.find_first_of(ends, p1 + name_len + pos);
		if (p2 == std::string::npos)
		{
			return -2;
		}
		std::string s_rsp = rsp;

		val = s_rsp.substr(p1 + name_len + pos, p2 - p1 - name_len - pos);
		return 0;
	}
private:
	std::vector<ReqInfo> _req_list;
};

class TestCase_IOT : public TestCase
{
public:
	TestCase_IOT(std::vector<TcpInfo> &ip_port)
	{
		_ip_port.assign(ip_port.begin(), ip_port.end());
	};
	virtual int Init(void*arg)
	{
		Client* client = NULL;
		client = (Client*)arg;

		for (size_t i = 0; i < _ip_port.size(); i++)
		{
			client->AddServer(_ip_port[i]._ip, _ip_port[i]._port);
		}
		return 0;
	};
	virtual ~TestCase_IOT()
	{
	};
	virtual int Execute(void*arg)
	{
		Client* client = NULL;
		client = (Client*)arg;
		std::string rsp;
		ReqInfo req_info = taf::TC_Singleton<ReqPool>::getInstance()->getReq(_user_id, _family_id, _token, _phone);

		int ret = client->sync_tcp_iot(req_info._req);
		if (ret != 0)
		{
			printf("SendAndRcv fail,ret:%d\n", ret);
			return -1;
		}
		return 0;
	}
	virtual int PreExecute(void *arg)
	{
		std::string login_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"um_login_pwd\",\"timestamp\":12345667,\"req_id\":123,\"params\":{\"phone\":\"$phone\",\"pwd\":\"96e79218965eb72c92a549dd5a330112\",\"os_type\":\"Android\"}}}\n";
		std::string family_info_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"fm_get_family_list\",\"timestamp\":12345667,\"req_id\":123}}\n";
		//todo:
		
		Client* client = NULL;
		client = (Client*)arg;
		std::string rsp;
		//替换变量
		UserAccountPool * pool = taf::TC_Singleton<UserAccountPool>::getInstance();
		_phone = pool->getIncre();
		std::string req = taf::TC_Common::replace(login_req, "$phone", _phone);
		unsigned short sockfd = 0;
		int ret = client->tcp_iot(req, rsp, sockfd);
		if (ret != 0)
		{
			printf("SendAndRcv1 fail,ret:%d\n", ret);
			return -1;
		}
		stringstream ss;
		ss
			<< "req:" << req << "\n"
			<< "rsp:" << rsp << "\n";
		taf::TC_Singleton<log_file>::getInstance()->log_error(ss.str());
		//判断成功失败
		_user_id = "123";
		_token = "vatea";
		_family_id = "123";
		taf::TC_Singleton<HandleInput>::getInstance()->insert(sockfd, "");
		return 0;
		std::string succ_flag = "code\":0";
		if (rsp.find(succ_flag) != std::string::npos)
		{
			std::string val;
			int ret = ReqPool::getVal(rsp, "user_id", val);
			if (ret != 0)
			{
				printf("getVal user_id fail!ret:%d\n", ret);
				return -2;
			}
			_user_id = val;

			ret = ReqPool::getVal(rsp, "\"token", val, false);
			if (ret != 0)
			{
				printf("getVal token fail!ret:%d\n", ret);
				return -3;
			}
			_token = val;

			ret = client->tcp_iot(family_info_req, rsp,sockfd);
			if (ret != 0)
			{
				printf("SendAndRcv1 fail,ret:%d\n", ret);
				return -4;
			}
			if (rsp.find(succ_flag) == std::string::npos)
			{
				printf("api fail!req:%s\n rsp:%s\n", family_info_req.c_str(), rsp.c_str());
				return -5;
			}
			ret = ReqPool::getVal(rsp, "family_id", val);
			if (ret != 0)
			{
				printf("getVal family_id fail!ret:%d\n", ret);
				return -6;
			}
			_family_id = val;
			printf("prepare req success!user_id:%s,family_id:%s\n", _user_id.c_str(), _family_id.c_str());
			return 0;
		}
		else {
			printf("rsp:%s\n", rsp.c_str());
			return -2;
		}


		return 0;
	}

private:
	// 在这里定义需要使用的成员变量
	std::vector<TcpInfo> _ip_port;
	std::string _user_id;
	std::string _family_id;
	std::string _token;
	std::string _phone;
};

void *rcv(void *argv)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
		fprintf(stderr, "set thread affinity failed\n");
	}

	time_t now;
	now = time(NULL);
	g_epoll.create(10240);
	do {
		if (time(NULL) > now)
		{
			now = time(NULL);
			taf::TC_Singleton<ReqQueue>::getInstance()->print(2000000);
			taf::TC_Singleton<TestFrameStatistic>::getInstance()->Output();
		}
		int fd_cnt = g_epoll.wait(1);
		if (fd_cnt <= 0)
		{
			usleep(1);
			continue;
		}
		//printf("recv:%d\n", fd_cnt);
		for (int i = 0; i < fd_cnt; i++)
		{
			std::string s;
			epoll_event ev = g_epoll.get(i);
			if (ev.events & EPOLLIN)
			{
				int _socket = ev.data.fd;
				int size_recv = 0;
				char chunk[LEN_MAXRECV] = {0};
				while (1)
				{
					bzero(chunk, sizeof(chunk));
					if ((size_recv = recv(_socket, chunk, sizeof(chunk), 0)) == -1)
					{
						if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							printf("recv timeout ...\n");
							break;
						}
						else if (errno == EINTR)
						{
							printf("interrupt by signal...\n");
							continue;
						}
						else if (errno == ENOENT)
						{
							printf("recv RST segement...\n");
							break;
						}
						else
						{
							printf("unknown error1!error:%d,msg:%s,fd:%d\n", errno, strerror(errno), _socket);
							//exit(1);
						}
					}
					else if (size_recv > 0)
					{
						std::map<unsigned short, std::string>::iterator it;
						
						if (taf::TC_Singleton<HandleInput>::getInstance()->find(_socket, it) != 0)
						{
							printf("error ,can not found _sockfd:%u\n", _socket);
							return NULL;
						}
						std::string str = it->second;
						s = string(chunk,size_recv);
						str.append(s);

						bool is_full = false;
						if (str[str.size() - 1] == '\n')
						{
							is_full = true;
							taf::TC_Singleton<HandleInput>::getInstance()->update(it, "");
						}
						std::vector<std::string> v = taf::TC_Common::sepstr<std::string>(str, "\n", false);
						for (size_t i = 0; i < v.size(); i++)
						{
							if ((i == v.size() - 1) && is_full == false)
							{
								taf::TC_Singleton<HandleInput>::getInstance()->update(it, v[i]);
								continue;
							}
							std::string req_id;
							unsigned int timecost = 0;
							ReqQueueItem req_item;
							if (ReqPool::getVal(v[i], "req_id", req_id) == 0)
							{
								if (taf::TC_Singleton<ReqQueue>::getInstance()->find(req_id, req_item, timecost) != 0)
								{
									taf::TC_Singleton<TestFrameStatistic>::getInstance()->IncreaseFailureCount();
									printf("fail=======can not found, rsp:%s\n",v[i].c_str());
									return NULL;
								}
								else {
									if (v[i].find(req_item._assert) != std::string::npos || v[i].find("code\":0") != std::string::npos)
									{
										taf::TC_Singleton<TestFrameStatistic>::getInstance()->IncreaseSuccessCount();
									}
									else {
										taf::TC_Singleton<TestFrameStatistic>::getInstance()->IncreaseFailureCount();
									}
									//printf("success==========req:%s,rsp:%s,assert:%s,costtime:%u\n", req_item._req.c_str(),v[i].c_str(), req_item._assert.c_str(), timecost);
								}
								taf::TC_Singleton<TestFrameStatistic>::getInstance()->UpdateDelay(timecost);
							}
							else {
								printf("can not found req_id,req:%s,rsp:%s,req_id:%s,s:%s\n",req_item._req.c_str(), v[i].c_str(), req_id.c_str(), s.c_str());
								//return NULL;
							}
						}
						if (strlen(chunk) == sizeof(chunk))
						{
							continue;
						}
						break;
					}
					else if (size_recv == 0) {
						printf("close conn\n");
						break;
					}
					else {
						printf("unknow error!\n");
						break;
					}
					//printf("received buff:%s\n",chunk);
					
				}
				//printf("recv:%s\n", sRecvBuffer.c_str());
				
				
			}
		}
	} while (1);
	return NULL;
};
pthread_t thread;
int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("Usage: %s file cmd\n", argv[0]);
		return 0;
	}
	std::string file = argv[1];
	unsigned int cmd = (unsigned int)atoi(argv[2]);

	LoadTestConfig config;
	int ret = config.Init(file);
	if (ret != 0)
	{
		printf("file not exist!%s,%d\n", file.c_str(), ret);
		return -1;
	}
	CmdInfo cmd_info;
	ret = config.get_config(cmd, cmd_info);
	if (ret != 0)
	{
		printf("cmd not in config file,cmd:%d\n", cmd);
		return -2;
	}

	printf("============================================\n");
	std::string s = cmd_info.to_str();
	printf("%s\n", s.c_str());
	printf("============================================\n");

	log_file * log = taf::TC_Singleton<log_file>::getInstance();
	log->Init(cmd_info.log_file, cmd_info.is_debug);
	log->log_error(cmd_info.to_str());
	//初始化预设参数

	taf::TC_Singleton<UserAccountPool>::getInstance()->Init(atoll(cmd_info._first_user_phone.c_str()));
	taf::TC_Singleton<ReqPool>::getInstance()->Init(cmd_info._req_list);
	// 测试框架实例
	unsigned int per_thread_limit = floor(cmd_info.frequence / cmd_info.thread_num);
	TestFrame tf;
	TestCase_IOT case1(cmd_info._tcp_info);
	tf.SetTestCase(&case1);
	if (cmd_info.times_limit != 0)
	{
		tf.SetRepeatCount(cmd_info.times_limit);
	}
	// 设置测试并发线程数
	tf.SetThreadNum(cmd_info.thread_num);
	// 设置测试持续时间，默认不限时间
	tf.SetDurationTime(cmd_info.load_test_time);
	// 设置频率限制，默认不限频率
	if (cmd_info.frequence != 0)
	{
		tf.SetFrequence(cmd_info.frequence);
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//int inher = PTHREAD_EXPLICIT_SCHED;
	//pthread_attr_setinheritsched(&attr, inher);
	int policy = SCHED_FIFO;
	pthread_attr_setschedpolicy(&attr, policy);

	int rc = pthread_create(&thread, &attr, rcv, NULL);

	if (rc) { 
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	tf.SetReqNum(per_thread_limit);
	// 设置时延阀值，用于统计时延健康度指标
	//tf.SetDelayThreshold(300);

	// 阻塞运行测试案例，并且输出测试结果
	tf.Run();

	return 0;
}


