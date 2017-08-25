#include <stdio.h>
#include <string>

// 包含测试框架头文件
#include "testframe.h"

#include "Client.h"
#include "../tcinclude/tc_http.h"
#include "../tcinclude/tc_singleton.h"
#include "../tcinclude/tc_common.h"
#include "loadConfig.h"
#include "testsynch.h"
#include "log.h"
#include <sys/time.h>
//static pthread_mutex_t *lockarray;

#define MAX_SDK_BUFFER_SIZE 1024
#define QUERY_SERVICE(cmd) client->RPC<cmd>(req, rsp)
#define QUERY_HTTP() client->http(httpBuffer,rspBuffer)

using namespace std;

class UserAccountPool {
public:
	UserAccountPool(){};
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
	void Init(const std::vector<ReqInfo> &req_list)
	{
		_req_list = req_list;
	}
	ReqInfo getReq(std::string &user_id, std::string &family_id)
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
		req_info._req = taf::TC_Common::replace(req_info._req, "$req_id", req_id);
		return req_info;
	}
	
	static int getVal(const std::string &rsp, const std::string &name,const size_t pos,std::string &val)
	{
		size_t p1 = rsp.find(name);
		if (p1 == std::string::npos)
		{
			printf("get user_id from rsp fail!\n");
			return -2;
		}
		size_t p2 = rsp.find_first_of(",", p1 + 1);
		if (p2 == std::string::npos)
		{
			printf("get user_id from rsp fail!\n");
			return -3;
		}
		std::string s_rsp = rsp;
		size_t name_len = name.size();
		val = s_rsp.substr(p1 + name_len+pos, p2 - p1 - name_len -pos);
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
			client->AddServer(_ip_port[i]._ip,_ip_port[i]._port);
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
		ReqInfo req_info = taf::TC_Singleton<ReqPool>::getInstance()->getReq(_user_id,_family_id);
		
		int ret = client->tcp_iot(req_info._req, rsp);
		if (ret != 0)
		{
			printf("SendAndRcv fail,ret:%d\n", ret);
			return -1;
		}

		//判断成功失败
		//std::string succ_flag = "code\":0";
		if (rsp.find(req_info._assert) != std::string::npos || rsp.find("code\":0") != std::string::npos)
		{
			stringstream ss;
			ss
				<< "success\n"
				<< "req:"<< req_info._req
				<< "rsp:" << rsp
				<< std::endl;
			taf::TC_Singleton<log_file>::getInstance()->log_debug(ss.str());
			return 0;
		}
		else {
			stringstream ss;
			ss
				<< "assert fail!\n"
				<< "req:"<< req_info._req
				<< "rsp:" << rsp
				<< "assert:" << req_info._assert
				<< std::endl;
			taf::TC_Singleton<log_file>::getInstance()->log_error(ss.str());
			return -2;
		}
	}
	virtual int PreExecute(void *arg)
	{
		std::string login_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"um_login_pwd\",\"timestamp\":12345667,\"req_id\":123,\"params\":{\"phone\":\"$phone\",\"pwd\":\"96e79218965eb72c92a549dd5a330112\",\"os_type\":\"Android\"}}}\n";
		std::string family_info_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"fm_get_family_list\",\"timestamp\":12345667,\"req_id\":123}}\n";

		Client* client = NULL;
		client = (Client*)arg;
		std::string rsp;
		//替换变量
		UserAccountPool * pool = taf::TC_Singleton<UserAccountPool>::getInstance();
		std::string req = taf::TC_Common::replace(login_req, "$phone", pool->getIncre());
		int ret = client->tcp_iot(req, rsp);
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
			int ret = ReqPool::getVal(rsp, "user_id", 2, val);
			if (ret != 0)
			{
				printf("getVal fail!ret:%d\n", ret);
				return -2;
			}
			_user_id = val;
			ret = client->tcp_iot(family_info_req, rsp);
			if (ret != 0)
			{
				printf("SendAndRcv1 fail,ret:%d\n", ret);
				return -3;
			}
			if (rsp.find(succ_flag) == std::string::npos)
			{
				printf("api fail!req:%s\n rsp:%s\n", family_info_req.c_str(),rsp.c_str());
				return -5;
			}
			ret = ReqPool::getVal(rsp, "family_id", 2, val);
			if (ret != 0)
			{
				printf("getVal fail!ret:%d\n", ret);
				return -6;
			}
			_family_id = val;
			printf("prepare req success!user_id:%s,family_id:%s\n",_user_id.c_str(),_family_id.c_str());
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
		printf("file not exist!%s\n", file.c_str());
		return -1;
	}
	CmdInfo cmd_info;
	ret = config.get_config(cmd,cmd_info);
	if (ret != 0)
	{
		printf("cmd not in config file,cmd:%d\n",cmd);
		return -2;
	}

	printf("============================================\n");
	std::string s = cmd_info.to_str();
	printf("%s\n", s.c_str());
	printf("============================================\n");

	log_file * log = taf::TC_Singleton<log_file>::getInstance();
	log->Init(cmd_info.log_file,cmd_info.is_debug);
	log->log_error(cmd_info.to_str());
	//初始化预设参数

	taf::TC_Singleton<UserAccountPool>::getInstance()->Init(atoll(cmd_info._first_user_phone.c_str()));
	taf::TC_Singleton<ReqPool>::getInstance()->Init(cmd_info._req_list);
	// 测试框架实例
	TestFrame tf;
	TestCase_IOT case1(cmd_info._tcp_info);
	tf.SetTestCase(&case1);
	/**
	switch (cmd)
	{
	case 10000:
		tf.SetTestCase(&case1);
		break;
	default:
		printf("ERROR CMD!\n");
		break;
	}
	**/
	// 测试案例实例
	//TestCase001 tc001;

	// 设置测试案例
	//tf.SetTestCase(&tc001);

	// 设置测试重复次数，默认不限次数
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

	// 设置时延阀值，用于统计时延健康度指标
	//tf.SetDelayThreshold(300);

	// 阻塞运行测试案例，并且输出测试结果
	tf.Run();

	return 0;
}


