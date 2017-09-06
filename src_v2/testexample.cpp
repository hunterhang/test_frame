#include <stdio.h>
#include <string>
#include <math.h>

// 包含测试框架头文件
#include "testframe.h"


#include "loadConfig.h"
#include "testsynch.h"
#include "log.h"
#include <sys/time.h>
#include <sys/epoll.h>
//static pthread_mutex_t *lockarray;

#define MAX_SDK_BUFFER_SIZE 1024
#define QUERY_SERVICE(cmd) client->RPC<cmd>(req, rsp)
#define QUERY_HTTP() client->http(httpBuffer,rspBuffer)


taf::TC_Epoller g_epoll(true);
Condition g_thread_cond;
MutexLock g_lock;

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


class ReqPool {
public:
	int Init(const std::vector<ReqInfo> &req_list)
	{
		_req_list = req_list;
		_is_weight_mode = false;
		unsigned int weight = 0;
		for (size_t i = 0; i < req_list.size(); i++)
		{
			weight += req_list[i]._weight;
		}
		if (weight != 100 || weight == 0)
		{
			printf("weight param config error!All values of weight must be 100/0 added together.current value is weight=%u\n",weight);
			return -1;
		}
		if (weight == 100)
		{
			_is_weight_mode = true;//权重模式
		}
		return 0;
	}
	ReqInfo getReq(std::string &user_id, std::string &family_id, std::string &token, std::string &phone)
	{
		struct timeval tv;
		gettimeofday(&tv, 0);
		srand(tv.tv_usec + atoi(user_id.c_str()));
		
		size_t index = 0;
		if (_is_weight_mode)
		{
			unsigned int weight = 0;
			unsigned int value = rand() % 100 + 1;
			size_t i = 0;
			for (; i < _req_list.size(); i++)
			{
				weight += _req_list[i]._weight;
				if (weight >= value)//找到了对应的权重
				{
					break;
				}
			}
			index = i;
		}
		else {
			index = rand() % _req_list.size();
		}
		
		std::string req_id = taf::TC_Singleton<ReqIdPool>::getInstance()->getIncre();
		ReqInfo req_info;
		req_info = _req_list[index];
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
	bool _is_weight_mode;
};


void *ThreadRcv(void *argv)
{

	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
		fprintf(stderr, "set thread affinity failed\n");
	}
	g_epoll.create(10240);
	//控制第1秒不准的问题，当发送完信息以后，再通知接收请求
	g_lock.Lock();
	g_thread_cond.Wait(g_lock);
	g_lock.Unlock();
	time_t now;
	now = time(NULL);
	
	unsigned int epoll_wait = 10;
	do {
		int fd_cnt = g_epoll.wait(epoll_wait);
		if (fd_cnt <= 0)
		{
			usleep(10);
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
				char chunk[LEN_MAXRECV] = { 0 };
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
						s = string(chunk, size_recv);
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
									printf("fail=======can not found, rsp:%s\n", v[i].c_str());
									return NULL;
								}
								else {
									taf::TC_Singleton<TestFrameStatistic>::getInstance()->UpdateDelay(timecost);
									if (v[i].find(req_item._assert) != std::string::npos || v[i].find("code\":0") != std::string::npos)
									{
										taf::TC_Singleton<TestFrameStatistic>::getInstance()->IncreaseSuccessCount();
									}
									else {
										//printf("fail,rsp:%s\n", v[i]);
										std::stringstream ss;
										ss
											<< "req:" << req_item._req
											<< "rsp:" << v[i]
											<< "assert:" << req_item._assert
											<< endl;
										taf::TC_Singleton<log_file>::getInstance()->log_error(ss.str());
										taf::TC_Singleton<TestFrameStatistic>::getInstance()->IncreaseFailureCount();
									}
									//printf("success==========req:%s,rsp:%s,assert:%s,costtime:%u\n", req_item._req.c_str(),v[i].c_str(), req_item._assert.c_str(), timecost);
								}
								
							}
							else {
								printf("can not found req_id,req:%s,rsp:%s,req_id:%s,s:%s\n", req_item._req.c_str(), v[i].c_str(), req_id.c_str(), s.c_str());
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

		if (time(NULL) > now)
		{
			now = time(NULL);
			taf::TC_Singleton<ReqQueue>::getInstance()->print(2000000);
			taf::TC_Singleton<TestFrameStatistic>::getInstance()->Output();
		}
	} while (1);
	return NULL;
}


class TestCase_IOT : public TestCase
{
public:
	TestCase_IOT()
	{
	};
	virtual int Init(const std::string &ip, const unsigned short &port)
	{
		_client.AddServer(ip, port);
		return 0;
	};
	virtual ~TestCase_IOT()
	{
	};
	virtual int Execute()
	{
		std::string rsp;
		ReqInfo req_info = taf::TC_Singleton<ReqPool>::getInstance()->getReq(_user_id, _family_id, _token, _phone);

		int ret = _client.sync_tcp_iot(req_info._req);
		if (ret != 0)
		{
			printf("SendAndRcv fail,ret:%d\n", ret);
			return -1;
		}
		return 0;
	}
	virtual int PreExecute()
	{
		std::string login_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"um_login_pwd\",\"timestamp\":12345667,\"req_id\":123,\"params\":{\"phone\":\"$phone\",\"pwd\":\"96e79218965eb72c92a549dd5a330112\",\"os_type\":\"Android\"}}}\n";
		std::string family_info_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"fm_get_family_list\",\"timestamp\":12345667,\"req_id\":123}}\n";
		//todo:

		std::string rsp;
		//替换变量
		UserAccountPool * pool = taf::TC_Singleton<UserAccountPool>::getInstance();
		_phone = pool->getIncre();
		std::string req = taf::TC_Common::replace(login_req, "$phone", _phone);
		unsigned short sockfd = 0;
		int ret = _client.tcp_iot(req, rsp, sockfd);
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
			ret = _client.tcp_iot(family_info_req, rsp, sockfd);
			ss.str("");
			ss
				<< "req:" << family_info_req << "\n"
				<< "rsp:" << rsp << "\n";
			taf::TC_Singleton<log_file>::getInstance()->log_error(ss.str());
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
				printf("getVal family_id fail!ret:%d,rsp:%s\n", ret,rsp.c_str());
				return -6;
			}
			_family_id = val;
			if (_family_id == "0" || _family_id == "")
			{
				printf("getVal family_id fail!user_id:%s,family_id:%s\n", _user_id.c_str(), _family_id.c_str());
				return -7;
			}
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
	std::string _user_id;
	std::string _family_id;
	std::string _token;
	std::string _phone;
	Client _client;
};


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
	if (taf::TC_Singleton<ReqPool>::getInstance()->Init(cmd_info._req_list) != 0)
	{
		printf("ReqPool Init fail!\n");
		exit(0);
	}
	// 测试框架实例
	TestFrame tf;
	for (unsigned int i = 0; i < cmd_info.conn_num; i++)
	{
		TestCase *case1 = new TestCase_IOT();
		unsigned int mode = i%cmd_info._tcp_info.size();
		int nRet = case1->Init(cmd_info._tcp_info[mode]._ip, cmd_info._tcp_info[mode]._port);
		if (nRet != 0)
		{
			printf("case init fail!nRet:%d\n", nRet);
			return -2;
		}
		else {
			//printf("case init success!\n");
		}
		tf.addObject(case1);
	}
	// 设置测试并发线程数
	tf.SetThreadNum(cmd_info.conn_num);
	// 设置测试持续时间，默认不限时间
	tf.SetDurationTime(cmd_info.load_test_time);
	// 设置频率限制，默认不限频率
	if (cmd_info.frequence != 0)
	{
		tf.SetFrequence(cmd_info.frequence);
	}
	pthread_t thread;
	int rc = pthread_create(&thread, NULL, ThreadRcv, NULL);

	if (rc) { 
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	// 阻塞运行测试案例，并且输出测试结果
	tf.Run();

	return 0;
}


